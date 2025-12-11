/// @file UriShortenerApp.h
/// @brief Main application class with factory pattern and message-based architecture

#pragma once

#include "Result.h"
#include "domain/ports/ILinkRepository.h"
#include "domain/ports/ICodeGenerator.h"
#include "application/use_cases/ShortenLink.h"
#include "application/use_cases/ResolveLink.h"
#include "application/use_cases/DeleteLink.h"
#include "UriShortenerMessageHandler.h"
#include "ObservableMessageHandler.h"
#include "UriShortenerRequestHandler.h"
#include "ObservableRequestHandler.h"
#include <StripedMessagePool.h>
#include "IRequest.h"
#include "IResponse.h"
#include "uri_shortener.pb.h"
#include <memory>
#include <string>
#include <thread>

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
 * Uses message-passing architecture with SEDA semantics.
 * Accepts proto config from uri_shortener::Config.
 */
class UriShortenerApp {
public:
    /// Optional overrides for testing (inject dependencies)
    struct Overrides {
        std::shared_ptr<domain::ILinkRepository> repository;
        std::shared_ptr<domain::ICodeGenerator> code_generator;
    };

    /**
     * @brief Factory method - creates and configures the app from proto config
     * @param config Protobuf configuration (loaded from JSON)
     * @param overrides Optional dependency overrides for testing
     * @return Ok(App) on success, Err(AppError) on failure
     */
    [[nodiscard]] static astra::Result<UriShortenerApp, AppError> create(
        const uri_shortener::Config& config,
        const Overrides& overrides = {});

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
        std::unique_ptr<UriShortenerMessageHandler> msg_handler,
        std::unique_ptr<ObservableMessageHandler> obs_msg_handler,
        std::unique_ptr<astra::execution::StripedMessagePool> pool,
        std::unique_ptr<UriShortenerRequestHandler> req_handler,
        std::unique_ptr<ObservableRequestHandler> obs_req_handler,
        std::unique_ptr<http2server::Server> server
    );

    // Error mapping
    static int domain_error_to_status(domain::DomainError err);
    static std::string domain_error_to_message(domain::DomainError err);

    // Domain components
    std::shared_ptr<domain::ILinkRepository> m_repo;
    std::shared_ptr<domain::ICodeGenerator> m_gen;
    std::shared_ptr<application::ShortenLink> m_shorten;
    std::shared_ptr<application::ResolveLink> m_resolve;
    std::shared_ptr<application::DeleteLink> m_delete;
    
    // Message-passing components (order matters for destruction)
    std::unique_ptr<UriShortenerMessageHandler> m_msg_handler;
    std::unique_ptr<ObservableMessageHandler> m_obs_msg_handler;
    std::unique_ptr<astra::execution::StripedMessagePool> m_pool;
    std::unique_ptr<UriShortenerRequestHandler> m_req_handler;
    std::unique_ptr<ObservableRequestHandler> m_obs_req_handler;
    
    // HTTP server
    std::unique_ptr<http2server::Server> m_server;
};

} // namespace url_shortener
