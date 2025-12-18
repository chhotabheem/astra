/// @file UriShortenerApp.cpp
/// @brief UriShortenerApp implementation with message-based architecture

#include "UriShortenerApp.h"
#include "InMemoryLinkRepository.h"
#include "ObservableLinkRepository.h"
#include "RandomCodeGenerator.h"
#include "Http2Server.h"
#include "Http2ClientPool.h"
#include <Provider.h>
#include <Log.h>
#include <Metrics.h>
#include <resilience/policy/LoadShedderPolicy.h>
#include <Http2Response.h>

namespace url_shortener {

UriShortenerApp::UriShortenerApp(
    std::shared_ptr<domain::ILinkRepository> repo,
    std::shared_ptr<domain::ICodeGenerator> gen,
    std::shared_ptr<application::ShortenLink> shorten,
    std::shared_ptr<application::ResolveLink> resolve,
    std::shared_ptr<application::DeleteLink> del,
    std::unique_ptr<astra::http2::Http2ClientPool> client_pool,
    std::shared_ptr<service::IDataServiceAdapter> data_adapter,
    std::unique_ptr<UriShortenerMessageHandler> msg_handler,
    std::unique_ptr<ObservableMessageHandler> obs_msg_handler,
    std::unique_ptr<astra::execution::StickyQueue> pool,
    std::unique_ptr<UriShortenerRequestHandler> req_handler,
    std::unique_ptr<ObservableRequestHandler> obs_req_handler,
    std::unique_ptr<astra::http2::Server> server,
    std::unique_ptr<astra::resilience::AtomicLoadShedder> load_shedder
)
    : m_repo(std::move(repo))
    , m_gen(std::move(gen))
    , m_shorten(std::move(shorten))
    , m_resolve(std::move(resolve))
    , m_delete(std::move(del))
    , m_client_pool(std::move(client_pool))
    , m_data_adapter(std::move(data_adapter))
    , m_msg_handler(std::move(msg_handler))
    , m_obs_msg_handler(std::move(obs_msg_handler))
    , m_pool(std::move(pool))
    , m_req_handler(std::move(req_handler))
    , m_obs_req_handler(std::move(obs_req_handler))
    , m_server(std::move(server))
    , m_load_shedder(std::move(load_shedder)) {}

UriShortenerApp::~UriShortenerApp() {
    // Stop pool before destruction
    if (m_pool) {
        m_pool->stop();
    }
}

UriShortenerApp::UriShortenerApp(UriShortenerApp&&) noexcept = default;
UriShortenerApp& UriShortenerApp::operator=(UriShortenerApp&&) noexcept = default;

astra::outcome::Result<UriShortenerApp, AppError> UriShortenerApp::create(
        const uri_shortener::Config& config,
        const Overrides& overrides) {
    
    // Extract bootstrap config
    const auto& bootstrap = config.bootstrap();
    std::string address = bootstrap.has_server() ? bootstrap.server().address() : "0.0.0.0";
    std::string port = bootstrap.has_server() ? std::to_string(bootstrap.server().port()) : "8080";
    size_t thread_count = (bootstrap.has_execution() && bootstrap.execution().has_shared_queue()) 
        ? bootstrap.execution().shared_queue().num_workers() : 4;
    
    // Validate config
    if (address.empty()) {
        return astra::outcome::Result<UriShortenerApp, AppError>::Err(AppError::InvalidConfig);
    }
    if (port.empty() || port == "0") {
        return astra::outcome::Result<UriShortenerApp, AppError>::Err(AppError::InvalidConfig);
    }

    // Initialize observability from proto config
    ::observability::Config obs_config;
    if (bootstrap.has_service()) {
        obs_config.set_service_name(bootstrap.service().name());
        obs_config.set_environment(bootstrap.service().environment());
    } else {
        obs_config.set_service_name("uri_shortener");
        obs_config.set_environment("development");
    }
    
    if (bootstrap.has_observability()) {
        const auto& obs_proto = bootstrap.observability();
        obs_config.set_service_version(obs_proto.service_version());
        obs_config.set_otlp_endpoint(obs_proto.otlp_endpoint());
        obs_config.set_metrics_enabled(obs_proto.metrics_enabled());
        obs_config.set_tracing_enabled(obs_proto.tracing_enabled());
        obs_config.set_logging_enabled(obs_proto.logging_enabled());
    } else {
        obs_config.set_service_version("1.0.0");
        obs_config.set_otlp_endpoint("http://localhost:4317");
    }
    
    if (!obs::init(obs_config)) {
        obs::warn("Observability initialization failed");
    }

    // Create repository with observability wrapper
    std::shared_ptr<domain::ILinkRepository> repo;
    if (overrides.repository) {
        repo = overrides.repository;
    } else {
        auto inner = std::make_shared<infrastructure::InMemoryLinkRepository>();
        repo = std::make_shared<infrastructure::ObservableLinkRepository>(inner);
    }
    
    auto gen = overrides.code_generator
        ? overrides.code_generator
        : std::make_shared<infrastructure::RandomCodeGenerator>();

    // Create use cases
    auto shorten = std::make_shared<application::ShortenLink>(repo, gen);
    auto resolve = std::make_shared<application::ResolveLink>(repo);
    auto del = std::make_shared<application::DeleteLink>(repo);

    // Create HTTP server from proto config
    auto server = std::make_unique<astra::http2::Server>(bootstrap.server());
    
    // Create Http2ClientPool for backend HTTP calls
    astra::http2::ClientConfig client_config;
    client_config.set_host("localhost");  // TODO: Get from config proto
    client_config.set_port(8080);
    client_config.set_pool_size(4);
    auto client_pool = std::make_unique<astra::http2::Http2ClientPool>(client_config);
    
    // Create HttpDataServiceAdapter as shared_ptr (MessageHandler takes shared ownership)
    auto data_adapter = std::make_shared<service::HttpDataServiceAdapter>(*client_pool);
    
    // Create MessageHandler with adapter (response_queue wired after pool creation)
    auto inner_msg_handler = std::make_unique<UriShortenerMessageHandler>(
        data_adapter,
        nullptr  // Will be set after pool creation
    );
    
    // Create observable wrapper
    auto obs_msg_handler = std::make_unique<ObservableMessageHandler>(*inner_msg_handler);
    
    // Create pool with observable handler
    auto pool = std::make_unique<astra::execution::StickyQueue>(
        thread_count, *obs_msg_handler);
    
    // Wire response_queue - use non-owning shared_ptr for pool
    auto response_queue = std::shared_ptr<astra::execution::IQueue>(
        pool.get(), [](auto*) {}  // Non-owning - App owns pool
    );
    inner_msg_handler->setResponseQueue(response_queue);
    
    // Create request handler chain
    auto req_handler = std::make_unique<UriShortenerRequestHandler>(*pool);
    auto obs_req_handler = std::make_unique<ObservableRequestHandler>(*req_handler);
    
    // Start the pool
    pool->start();
    
    // Create load shedder from runtime config
    size_t max_concurrent = 1000;  // default
    if (config.has_runtime() && config.runtime().has_load_shedder() &&
        config.runtime().load_shedder().max_concurrent_requests() > 0) {
        max_concurrent = config.runtime().load_shedder().max_concurrent_requests();
    }
    auto load_shedder_policy = astra::resilience::LoadShedderPolicy::create(
        max_concurrent, "uri_shortener");
    auto load_shedder = std::make_unique<astra::resilience::AtomicLoadShedder>(
        std::move(load_shedder_policy));

    return astra::outcome::Result<UriShortenerApp, AppError>::Ok(UriShortenerApp(
        std::move(repo),
        std::move(gen),
        std::move(shorten),
        std::move(resolve),
        std::move(del),
        std::move(client_pool),
        std::move(data_adapter),
        std::move(inner_msg_handler),
        std::move(obs_msg_handler),
        std::move(pool),
        std::move(req_handler),
        std::move(obs_req_handler),
        std::move(server),
        std::move(load_shedder)
    ));
}

int UriShortenerApp::run() {
    // Register load shedder metrics
    auto accepted = obs::counter("load_shedder.accepted");
    auto rejected = obs::counter("load_shedder.rejected");
    
    auto resilient = [this, accepted, rejected](std::shared_ptr<astra::router::IRequest> req, 
                                                  std::shared_ptr<astra::router::IResponse> res) {
        auto guard = m_load_shedder->try_acquire();
        if (!guard) {
            rejected.inc();
            obs::warn("Load shedder rejected request", {
                {"current", std::to_string(m_load_shedder->current_count())},
                {"max", std::to_string(m_load_shedder->max_concurrent())}
            });
            res->set_status(503);
            res->set_header("Content-Type", "application/json");
            res->set_header("Retry-After", "1");
            res->write(R"({"error": "Service overloaded"})");
            res->close();
            return;
        }
        
        accepted.inc();
        
        // Cast to concrete type for scoped resource
        auto http_res = std::dynamic_pointer_cast<astra::http2::ServerResponse>(res);
        if (http_res) {
            http_res->add_scoped_resource(
                std::make_unique<astra::resilience::LoadShedderGuard>(std::move(*guard)));
        }
        
        m_obs_req_handler->handle(req, res);
    };
    
    // Register handlers with resilience wrapper
    m_server->router().post("/shorten", resilient);
    m_server->router().get("/:code", resilient);
    m_server->router().del("/:code", resilient);
    
    // Health check bypasses load shedding
    m_server->router().get("/health", [](std::shared_ptr<astra::router::IRequest> /*req*/, 
                                          std::shared_ptr<astra::router::IResponse> res) {
        res->set_status(200);
        res->set_header("Content-Type", "application/json");
        res->write(R"({"status": "ok"})");
        res->close();
    });

    obs::info("URI Shortener listening");
    obs::info("Using message-based architecture", {{"workers", std::to_string(m_pool->worker_count())}});
    obs::info("Load shedder enabled", {{"max_concurrent", std::to_string(m_load_shedder->max_concurrent())}});
    m_server->run();
    return 0;
}

// =============================================================================
// Error Mapping
// =============================================================================

int UriShortenerApp::domain_error_to_status(domain::DomainError err) {
    switch (err) {
        case domain::DomainError::InvalidShortCode:
        case domain::DomainError::InvalidUrl:
            return 400;
        case domain::DomainError::LinkNotFound:
            return 404;
        case domain::DomainError::LinkExpired:
            return 410;
        case domain::DomainError::LinkAlreadyExists:
            return 409;
        case domain::DomainError::CodeGenerationFailed:
            return 500;
        default:
            return 500;
    }
}

std::string UriShortenerApp::domain_error_to_message(domain::DomainError err) {
    switch (err) {
        case domain::DomainError::InvalidShortCode:
            return "Invalid short code";
        case domain::DomainError::InvalidUrl:
            return "Invalid URL";
        case domain::DomainError::LinkNotFound:
            return "Link not found";
        case domain::DomainError::LinkExpired:
            return "Link has expired";
        case domain::DomainError::LinkAlreadyExists:
            return "Link already exists";
        case domain::DomainError::CodeGenerationFailed:
            return "Failed to generate code";
        default:
            return "Internal error";
    }
}

} // namespace url_shortener
