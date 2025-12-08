/// @file UriShortenerApp.cpp
/// @brief UriShortenerApp implementation with message-based architecture

#include "UriShortenerApp.h"
#include "infrastructure/persistence/InMemoryLinkRepository.h"
#include "infrastructure/observability/ObservableLinkRepository.h"
#include "infrastructure/generators/RandomCodeGenerator.h"
#include "Http2Server.h"
#include <obs/ConsoleBackend.h>
#include <obs/Observability.h>
#include <iostream>

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

astra::Result<UriShortenerApp, AppError> UriShortenerApp::create(const Config& config) {
    // Validate config
    if (config.address.empty()) {
        return astra::Result<UriShortenerApp, AppError>::Err(AppError::InvalidConfig);
    }
    if (config.port.empty()) {
        return astra::Result<UriShortenerApp, AppError>::Err(AppError::InvalidConfig);
    }

    // Initialize observability backend
    obs::set_backend(std::make_unique<obs::ConsoleBackend>());

    // Create repository with observability wrapper
    std::shared_ptr<domain::ILinkRepository> repo;
    if (config.repository) {
        repo = config.repository;
    } else {
        auto inner = std::make_shared<infrastructure::InMemoryLinkRepository>();
        repo = std::make_shared<infrastructure::ObservableLinkRepository>(inner);
    }
    
    auto gen = config.code_generator
        ? config.code_generator
        : std::make_shared<infrastructure::RandomCodeGenerator>();

    // Create use cases
    auto shorten = std::make_shared<application::ShortenLink>(repo, gen);
    auto resolve = std::make_shared<application::ResolveLink>(repo);
    auto del = std::make_shared<application::DeleteLink>(repo);

    // Create HTTP server
    auto server = std::make_unique<http2server::Server>(config.address, config.port);
    
    // Create message handler chain
    // Handler processes synchronously (no pool back-reference needed)
    auto inner_msg_handler = std::make_unique<UriShortenerMessageHandler>(
        shorten, resolve, del);
    
    // Create observable wrapper
    auto obs_msg_handler = std::make_unique<ObservableMessageHandler>(*inner_msg_handler);
    
    // Create pool with observable handler
    size_t thread_count = config.thread_count > 0 ? config.thread_count : 4;
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

    std::cout << "URI Shortener listening on port " << "...\n";
    std::cout << "Using message-based architecture with " << m_pool->thread_count() << " workers\n";
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
