#pragma once

#include "ILinkRepository.h"
#include "ICodeGenerator.h"
#include "ShortenLink.h"
#include "ResolveLink.h"
#include "DeleteLink.h"
#include "UriShortenerMessageHandler.h"
#include "ObservableMessageHandler.h"
#include "UriShortenerRequestHandler.h"
#include "ObservableRequestHandler.h"
#include "IDataServiceAdapter.h"
#include <StickyQueue.h>
#include <resilience/impl/AtomicLoadShedder.h>
#include <memory>

namespace astra::http2 { class Http2Server; class Http2Client; }
namespace astra::service_discovery { class IServiceResolver; }

namespace uri_shortener {

class UriShortenerApp {
public:
    UriShortenerApp(
        std::shared_ptr<domain::ILinkRepository> repo,
        std::shared_ptr<domain::ICodeGenerator> gen,
        std::shared_ptr<application::ShortenLink> shorten,
        std::shared_ptr<application::ResolveLink> resolve,
        std::shared_ptr<application::DeleteLink> del,
        std::unique_ptr<astra::http2::Http2Client> http2_client,
        std::unique_ptr<astra::service_discovery::IServiceResolver> resolver,
        std::shared_ptr<service::IDataServiceAdapter> data_adapter,
        std::unique_ptr<UriShortenerMessageHandler> msg_handler,
        std::unique_ptr<ObservableMessageHandler> obs_msg_handler,
        std::unique_ptr<astra::execution::StickyQueue> pool,
        std::unique_ptr<UriShortenerRequestHandler> req_handler,
        std::unique_ptr<ObservableRequestHandler> obs_req_handler,
        std::unique_ptr<astra::http2::Http2Server> server,
        std::unique_ptr<astra::resilience::AtomicLoadShedder> load_shedder
    );

    [[nodiscard]] int run();

    UriShortenerApp(UriShortenerApp&&) noexcept;
    UriShortenerApp& operator=(UriShortenerApp&&) noexcept;
    ~UriShortenerApp();

    UriShortenerApp(const UriShortenerApp&) = delete;
    UriShortenerApp& operator=(const UriShortenerApp&) = delete;

private:
    std::shared_ptr<domain::ILinkRepository> m_repo;
    std::shared_ptr<domain::ICodeGenerator> m_gen;
    std::shared_ptr<application::ShortenLink> m_shorten;
    std::shared_ptr<application::ResolveLink> m_resolve;
    std::shared_ptr<application::DeleteLink> m_delete;
    
    std::unique_ptr<astra::http2::Http2Client> m_http2_client;
    std::unique_ptr<astra::service_discovery::IServiceResolver> m_resolver;
    std::shared_ptr<service::IDataServiceAdapter> m_data_adapter;
    
    std::unique_ptr<UriShortenerMessageHandler> m_msg_handler;
    std::unique_ptr<ObservableMessageHandler> m_obs_msg_handler;
    std::unique_ptr<astra::execution::StickyQueue> m_pool;
    std::unique_ptr<UriShortenerRequestHandler> m_req_handler;
    std::unique_ptr<ObservableRequestHandler> m_obs_req_handler;
    
    std::unique_ptr<astra::http2::Http2Server> m_server;
    
    std::unique_ptr<astra::resilience::AtomicLoadShedder> m_load_shedder;
};

} // namespace uri_shortener


