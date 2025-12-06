/// @file UriShortenerApp.h
/// @brief Main application class with factory pattern

#pragma once

#include "Result.h"
#include "domain/ports/ILinkRepository.h"
#include "domain/ports/ICodeGenerator.h"
#include "application/use_cases/ShortenLink.h"
#include "application/use_cases/ResolveLink.h"
#include "application/use_cases/DeleteLink.h"
#include "IRequest.h"
#include "IResponse.h"
#include <memory>
#include <string>

// Forward declaration
namespace http2server { class Server; }

namespace url_shortener {

/// Application-level errors
enum class AppError {
    InvalidConfig,
    ServerCreationFailed
};

/**
 * @brief URI Shortener Application
 * 
 * Factory pattern with static create() method.
 * Owns server, repository, and use cases.
 */
class UriShortenerApp {
public:
    /// Configuration for the application
    struct Config {
        std::string address = "0.0.0.0";
        std::string port = "8080";
        
        // Optional: inject dependencies (for testing)
        std::shared_ptr<domain::ILinkRepository> repository;
        std::shared_ptr<domain::ICodeGenerator> code_generator;
    };

    /**
     * @brief Factory method - creates and configures the app
     * @param config Application configuration
     * @return Ok(App) on success, Err(AppError) on failure
     */
    [[nodiscard]] static astra::Result<UriShortenerApp, AppError> create(const Config& config);

    /**
     * @brief Run the application (blocking)
     * @return Exit code (0 = success)
     */
    [[nodiscard]] int run();

    /// Moveable
    UriShortenerApp(UriShortenerApp&&) noexcept;
    UriShortenerApp& operator=(UriShortenerApp&&) noexcept;

    ~UriShortenerApp();

    /// Not copyable
    UriShortenerApp(const UriShortenerApp&) = delete;
    UriShortenerApp& operator=(const UriShortenerApp&) = delete;

private:
    UriShortenerApp(
        std::shared_ptr<domain::ILinkRepository> repo,
        std::shared_ptr<domain::ICodeGenerator> gen,
        std::shared_ptr<application::ShortenLink> shorten,
        std::shared_ptr<application::ResolveLink> resolve,
        std::shared_ptr<application::DeleteLink> del,
        std::unique_ptr<http2server::Server> server
    );

    // HTTP Handlers
    void handle_shorten(router::IRequest& req, router::IResponse& res);
    void handle_resolve(router::IRequest& req, router::IResponse& res);
    void handle_delete(router::IRequest& req, router::IResponse& res);
    void handle_health(router::IRequest& req, router::IResponse& res);

    // Error mapping
    static int domain_error_to_status(domain::DomainError err);
    static std::string domain_error_to_message(domain::DomainError err);

    std::shared_ptr<domain::ILinkRepository> m_repo;
    std::shared_ptr<domain::ICodeGenerator> m_gen;
    std::shared_ptr<application::ShortenLink> m_shorten;
    std::shared_ptr<application::ResolveLink> m_resolve;
    std::shared_ptr<application::DeleteLink> m_delete;
    std::unique_ptr<http2server::Server> m_server;
};

} // namespace url_shortener

