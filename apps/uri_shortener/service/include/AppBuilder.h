#pragma once

#include "Result.h"
#include "UriShortenerApp.h"
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
#include "uri_shortener.pb.h"
#include <resilience/impl/AtomicLoadShedder.h>
#include <memory>
#include <string>

namespace astra::http2 { class Http2Server; class Http2Client; }
namespace astra::service_discovery { class IServiceResolver; }

namespace uri_shortener {

enum class AppError {
    InvalidConfig,
    ServerCreationFailed
};

inline std::string to_string(AppError err) {
    switch (err) {
        case AppError::InvalidConfig: return "InvalidConfig";
        case AppError::ServerCreationFailed: return "ServerCreationFailed";
    }
    return "Unknown";
}

class AppBuilder {
public:
    static int start(int argc, char* argv[], const std::string& defaultConfigPath);
    
    explicit AppBuilder(const Config& config);
    ~AppBuilder();
    
    AppBuilder& domain();
    AppBuilder& backend();
    AppBuilder& messaging();
    AppBuilder& resilience();
    
    astra::outcome::Result<UriShortenerApp, AppError> build();

private:
    AppBuilder& repo();
    AppBuilder& codeGen();
    AppBuilder& useCases();
    
    AppBuilder& httpClient();
    AppBuilder& serviceResolver();
    AppBuilder& dataAdapter();
    
    AppBuilder& msgHandler();
    AppBuilder& pool();
    AppBuilder& reqHandler();
    AppBuilder& wrapObservable();
    
    AppBuilder& loadShedder();
    
    void initObservability();

    const Config& m_config;
    
    std::shared_ptr<domain::ILinkRepository> m_repo;
    std::shared_ptr<domain::ICodeGenerator> m_gen;
    std::shared_ptr<application::ShortenLink> m_shorten;
    std::shared_ptr<application::ResolveLink> m_resolve;
    std::shared_ptr<application::DeleteLink> m_delete;
    
    std::unique_ptr<astra::http2::Http2Client> m_http_client;
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
