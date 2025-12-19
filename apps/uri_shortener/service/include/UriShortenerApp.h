/// @file UriShortenerApp.h
/// @brief Main application class with factory pattern and message-based architecture

#pragma once

#include "Result.h"
#include "ILinkRepository.h"
#include "ICodeGenerator.h"
#include "ShortenLink.h"
#include "ResolveLink.h"
#include "DeleteLink.h"
#include "UriShortenerMessageHandler.h"
#include "ObservableMessageHandler.h"
#include "UriShortenerRequestHandler.h"
#include "ObservableRequestHandler.h"
#include "HttpDataServiceAdapter.h"
#include <StickyQueue.h>
#include "IRequest.h"
#include "IResponse.h"
#include "uri_shortener.pb.h"
#include <resilience/impl/AtomicLoadShedder.h>
#include <memory>
#include <string>
#include <thread>

// Forward declarations
namespace astra::http2 { class Http2Server; class Http2ClientPool; }

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
 */
class UriShortenerApp {
public:
    struct Overrides {
        std::shared_ptr<domain::ILinkRepository> repository;
        std::shared_ptr<domain::ICodeGenerator> code_generator;
    };

    [[nodiscard]] static astra::outcome::Result<UriShortenerApp, AppError> create(
        const uri_shortener::Config& config,
        const Overrides& overrides = {});

    [[nodiscard]] int run();

    UriShortenerApp(UriShortenerApp&&) noexcept;
    UriShortenerApp& operator=(UriShortenerApp&&) noexcept;
    ~UriShortenerApp();

    UriShortenerApp(const UriShortenerApp&) = delete;
    UriShortenerApp& operator=(const UriShortenerApp&) = delete;

private:
    UriShortenerApp(
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
        std::unique_ptr<astra::http2::Http2Server> server,
        std::unique_ptr<astra::resilience::AtomicLoadShedder> load_shedder
    );

    static int domain_error_to_status(domain::DomainError err);
    static std::string domain_error_to_message(domain::DomainError err);

    // Domain components
    std::shared_ptr<domain::ILinkRepository> m_repo;
    std::shared_ptr<domain::ICodeGenerator> m_gen;
    std::shared_ptr<application::ShortenLink> m_shorten;
    std::shared_ptr<application::ResolveLink> m_resolve;
    std::shared_ptr<application::DeleteLink> m_delete;
    
    // Backend HTTP client (destruction order: adapter uses pool)
    std::unique_ptr<astra::http2::Http2ClientPool> m_client_pool;
    std::shared_ptr<service::IDataServiceAdapter> m_data_adapter;
    
    // Message-passing components
    std::unique_ptr<UriShortenerMessageHandler> m_msg_handler;
    std::unique_ptr<ObservableMessageHandler> m_obs_msg_handler;
    std::unique_ptr<astra::execution::StickyQueue> m_pool;
    std::unique_ptr<UriShortenerRequestHandler> m_req_handler;
    std::unique_ptr<ObservableRequestHandler> m_obs_req_handler;
    
    // HTTP server
    std::unique_ptr<astra::http2::Http2Server> m_server;
    
    // Resilience
    std::unique_ptr<astra::resilience::AtomicLoadShedder> m_load_shedder;
};

} // namespace url_shortener

