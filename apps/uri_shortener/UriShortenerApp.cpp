/// @file UriShortenerApp.cpp
/// @brief UriShortenerApp implementation with message-based architecture

#include "UriShortenerApp.h"
#include "infrastructure/persistence/InMemoryLinkRepository.h"
#include "infrastructure/observability/ObservableLinkRepository.h"
#include "infrastructure/generators/RandomCodeGenerator.h"
#include "Http2Server.h"
#include <Provider.h>
#include <Config.h>
#include <Log.h>

namespace url_shortener {

UriShortenerApp::UriShortenerApp(
    std::shared_ptr<domain::ILinkRepository> repo,
    std::shared_ptr<domain::ICodeGenerator> gen,
    std::shared_ptr<application::ShortenLink> shorten,
    std::shared_ptr<application::ResolveLink> resolve,
    std::shared_ptr<application::DeleteLink> del,
    std::unique_ptr<UriShortenerMessageHandler> msg_handler,
    std::unique_ptr<ObservableMessageHandler> obs_msg_handler,
    std::unique_ptr<astra::execution::StripedMessagePool> pool,
    std::unique_ptr<UriShortenerRequestHandler> req_handler,
    std::unique_ptr<ObservableRequestHandler> obs_req_handler,
    std::unique_ptr<http2server::Server> server
)
    : m_repo(std::move(repo))
    , m_gen(std::move(gen))
    , m_shorten(std::move(shorten))
    , m_resolve(std::move(resolve))
    , m_delete(std::move(del))
    , m_msg_handler(std::move(msg_handler))
    , m_obs_msg_handler(std::move(obs_msg_handler))
    , m_pool(std::move(pool))
    , m_req_handler(std::move(req_handler))
    , m_obs_req_handler(std::move(obs_req_handler))
    , m_server(std::move(server)) {}

UriShortenerApp::~UriShortenerApp() {
    // Stop pool before destruction
    if (m_pool) {
        m_pool->stop();
    }
}

UriShortenerApp::UriShortenerApp(UriShortenerApp&&) noexcept = default;
UriShortenerApp& UriShortenerApp::operator=(UriShortenerApp&&) noexcept = default;

astra::Result<UriShortenerApp, AppError> UriShortenerApp::create(
        const uri_shortener::Config& config,
        const Overrides& overrides) {
    
    // Extract bootstrap config
    const auto& bootstrap = config.bootstrap();
    std::string address = bootstrap.has_server() ? bootstrap.server().address() : "0.0.0.0";
    std::string port = bootstrap.has_server() ? std::to_string(bootstrap.server().port()) : "8080";
    size_t thread_count = bootstrap.has_threading() ? bootstrap.threading().worker_threads() : 4;
    
    // Validate config
    if (address.empty()) {
        return astra::Result<UriShortenerApp, AppError>::Err(AppError::InvalidConfig);
    }
    if (port.empty() || port == "0") {
        return astra::Result<UriShortenerApp, AppError>::Err(AppError::InvalidConfig);
    }

    // Initialize observability from proto config (C++17 compatible - no designated initializers)
    obs::InitParams obs_params;
    if (bootstrap.has_service()) {
        obs_params.service_name = bootstrap.service().name();
        obs_params.environment = bootstrap.service().environment();
    } else {
        obs_params.service_name = "uri_shortener";
        obs_params.environment = "development";
    }
    
    if (config.has_operational() && config.operational().has_observability()) {
        const auto& obs_config = config.operational().observability();
        obs_params.service_version = obs_config.service_version();
        obs_params.otlp_endpoint = obs_config.otlp_endpoint();
        obs_params.enable_metrics = obs_config.metrics_enabled();
        obs_params.enable_tracing = obs_config.tracing_enabled();
        obs_params.enable_logging = obs_config.logging_enabled();
    } else {
        obs_params.service_version = "1.0.0";
        obs_params.otlp_endpoint = "http://localhost:4317";
    }
    
    if (!obs::init(obs_params)) {
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

    // Create HTTP server
    auto server = std::make_unique<http2server::Server>(address, port);
    
    // Create message handler chain
    auto inner_msg_handler = std::make_unique<UriShortenerMessageHandler>(
        shorten, resolve, del);
    
    // Create observable wrapper
    auto obs_msg_handler = std::make_unique<ObservableMessageHandler>(*inner_msg_handler);
    
    // Create pool with observable handler
    auto pool = std::make_unique<astra::execution::StripedMessagePool>(
        thread_count, *obs_msg_handler);
    
    // Create request handler chain
    auto req_handler = std::make_unique<UriShortenerRequestHandler>(*pool);
    auto obs_req_handler = std::make_unique<ObservableRequestHandler>(*req_handler);
    
    // Start the pool
    pool->start();

    return astra::Result<UriShortenerApp, AppError>::Ok(UriShortenerApp(
        std::move(repo),
        std::move(gen),
        std::move(shorten),
        std::move(resolve),
        std::move(del),
        std::move(inner_msg_handler),
        std::move(obs_msg_handler),
        std::move(pool),
        std::move(req_handler),
        std::move(obs_req_handler),
        std::move(server)
    ));
}

int UriShortenerApp::run() {
    // Register single unified handler that delegates to message-based flow
    m_server->router().post("/shorten", [this](router::IRequest& req, router::IResponse& res) {
        m_obs_req_handler->handle(req, res);
    });
    m_server->router().get("/:code", [this](router::IRequest& req, router::IResponse& res) {
        m_obs_req_handler->handle(req, res);
    });
    m_server->router().del("/:code", [this](router::IRequest& req, router::IResponse& res) {
        m_obs_req_handler->handle(req, res);
    });
    m_server->router().get("/health", [](router::IRequest& /*req*/, router::IResponse& res) {
        res.set_status(200);
        res.set_header("Content-Type", "application/json");
        res.write(R"({"status": "ok"})");
        res.close();
    });

    obs::info("URI Shortener listening");
    obs::info("Using message-based architecture", {{"workers", std::to_string(m_pool->thread_count())}});
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
