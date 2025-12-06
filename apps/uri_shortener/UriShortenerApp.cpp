/// @file UriShortenerApp.cpp
/// @brief UriShortenerApp implementation

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
    std::unique_ptr<http2server::Server> server
)
    : m_repo(std::move(repo))
    , m_gen(std::move(gen))
    , m_shorten(std::move(shorten))
    , m_resolve(std::move(resolve))
    , m_delete(std::move(del))
    , m_server(std::move(server)) {}

UriShortenerApp::~UriShortenerApp() = default;
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
        // Use provided (already wrapped or test mock)
        repo = config.repository;
    } else {
        // Create InMemory and wrap with observability
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

    // Create app instance (routes registered in run())
    return astra::Result<UriShortenerApp, AppError>::Ok(UriShortenerApp(
        std::move(repo),
        std::move(gen),
        std::move(shorten),
        std::move(resolve),
        std::move(del),
        std::move(server)
    ));
}

int UriShortenerApp::run() {
    // Register routes (using this pointer, not captured reference)
    m_server->router().get("/health", [this](router::IRequest& req, router::IResponse& res) {
        handle_health(req, res);
    });
    m_server->router().post("/shorten", [this](router::IRequest& req, router::IResponse& res) {
        handle_shorten(req, res);
    });
    m_server->router().get("/:code", [this](router::IRequest& req, router::IResponse& res) {
        handle_resolve(req, res);
    });
    m_server->router().del("/:code", [this](router::IRequest& req, router::IResponse& res) {
        handle_delete(req, res);
    });

    std::cout << "URI Shortener listening on port...\n";
    m_server->run();
    return 0;
}

// =============================================================================
// HTTP Handlers
// =============================================================================

void UriShortenerApp::handle_shorten(router::IRequest& req, router::IResponse& res) {
    // Parse JSON body: {"url": "https://example.com"}
    auto body = req.body();
    // Simple JSON extraction (minimal)
    auto url_start = body.find("\"url\"");
    if (url_start == std::string_view::npos) {
        res.set_status(400);
        res.set_header("Content-Type", "application/json");
        res.write(R"({"error": "Missing 'url' field"})");
        res.close();
        return;
    }
    
    auto value_start = body.find(':', url_start);
    auto quote_start = body.find('"', value_start);
    auto quote_end = body.find('"', quote_start + 1);
    if (quote_start == std::string_view::npos || quote_end == std::string_view::npos) {
        res.set_status(400);
        res.set_header("Content-Type", "application/json");
        res.write(R"({"error": "Invalid JSON"})");
        res.close();
        return;
    }
    
    std::string url(body.substr(quote_start + 1, quote_end - quote_start - 1));
    
    application::ShortenLink::Input input{.original_url = url};
    auto result = m_shorten->execute(input);
    
    res.set_header("Content-Type", "application/json");
    if (result.is_err()) {
        res.set_status(domain_error_to_status(result.error()));
        res.write("{\"error\": \"" + domain_error_to_message(result.error()) + "\"}");
    } else {
        auto output = result.value();
        res.set_status(201);
        res.write("{\"short_code\": \"" + output.short_code + "\", \"original_url\": \"" + output.original_url + "\"}");
    }
    res.close();
}

void UriShortenerApp::handle_resolve(router::IRequest& req, router::IResponse& res) {
    auto code = req.path_param("code");
    if (code.empty()) {
        res.set_status(400);
        res.set_header("Content-Type", "application/json");
        res.write(R"({"error": "Missing code"})");
        res.close();
        return;
    }

    application::ResolveLink::Input input{.short_code = std::string(code)};
    auto result = m_resolve->execute(input);

    res.set_header("Content-Type", "application/json");
    if (result.is_err()) {
        res.set_status(domain_error_to_status(result.error()));
        res.write("{\"error\": \"" + domain_error_to_message(result.error()) + "\"}");
    } else {
        res.set_status(200);
        res.write("{\"original_url\": \"" + result.value().original_url + "\"}");
    }
    res.close();
}

void UriShortenerApp::handle_delete(router::IRequest& req, router::IResponse& res) {
    auto code = req.path_param("code");
    if (code.empty()) {
        res.set_status(400);
        res.set_header("Content-Type", "application/json");
        res.write(R"({"error": "Missing code"})");
        res.close();
        return;
    }

    application::DeleteLink::Input input{.short_code = std::string(code)};
    auto result = m_delete->execute(input);

    res.set_header("Content-Type", "application/json");
    if (result.is_err()) {
        res.set_status(domain_error_to_status(result.error()));
        res.write("{\"error\": \"" + domain_error_to_message(result.error()) + "\"}");
    } else {
        res.set_status(204);  // No Content
    }
    res.close();
}

void UriShortenerApp::handle_health(router::IRequest& /*req*/, router::IResponse& res) {
    res.set_status(200);
    res.set_header("Content-Type", "application/json");
    res.write(R"({"status": "ok"})");
    res.close();
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

