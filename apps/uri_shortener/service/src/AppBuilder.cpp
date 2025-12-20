#include "AppBuilder.h"
#include "UriShortenerApp.h"
#include "InMemoryLinkRepository.h"
#include "ObservableLinkRepository.h"
#include "RandomCodeGenerator.h"
#include "HttpDataServiceAdapter.h"
#include "Http2Server.h"
#include "Http2Client.h"
#include "StaticServiceResolver.h"
#include "ProtoConfigLoader.h"
#include <Provider.h>
#include <Log.h>
#include <resilience/policy/LoadShedderPolicy.h>
#include <cstdlib>

namespace uri_shortener {

int AppBuilder::start(int argc, char* argv[], const std::string& defaultConfigPath) {
    ::observability::Config bootstrap_obs;
    bootstrap_obs.set_service_name("uri-shortener");
    bootstrap_obs.set_service_version("1.0.0");
    bootstrap_obs.set_environment("bootstrap");
    obs::init(bootstrap_obs);

    std::string config_path = defaultConfigPath;
    if (argc > 1) {
        config_path = argv[1];
    }
    if (const char* env_config = std::getenv("URI_SHORTENER_CONFIG")) {
        config_path = env_config;
    }

    obs::info("Loading config", {{"path", config_path}});

    auto load_result = ProtoConfigLoader::loadFromFile(config_path);
    if (!load_result.success) {
        obs::error("Failed to load config", 
            {{"path", config_path}, {"error", load_result.error}});
        return 1;
    }

    auto result = AppBuilder(load_result.config)
        .domain()
        .backend()
        .messaging()
        .resilience()
        .build();

    if (result.is_err()) {
        obs::error("Failed to start URI Shortener", 
            {{"error", to_string(result.error())}});
        return 1;
    }

    return result.value().run();
}

AppBuilder::AppBuilder(const Config& config) : m_config(config) {}

AppBuilder::~AppBuilder() = default;

AppBuilder& AppBuilder::domain() {
    return repo()
          .codeGen()
          .useCases();
}

AppBuilder& AppBuilder::backend() {
    return httpClient()
          .serviceResolver()
          .dataAdapter();
}

AppBuilder& AppBuilder::messaging() {
    return msgHandler()
          .pool()
          .reqHandler()
          .wrapObservable();
}

AppBuilder& AppBuilder::resilience() {
    return loadShedder();
}

AppBuilder& AppBuilder::repo() {
    m_repo = std::make_shared<infrastructure::ObservableLinkRepository>(
        std::make_shared<infrastructure::InMemoryLinkRepository>());
    return *this;
}

AppBuilder& AppBuilder::codeGen() {
    m_gen = std::make_shared<infrastructure::RandomCodeGenerator>();
    return *this;
}

AppBuilder& AppBuilder::useCases() {
    m_shorten = std::make_shared<application::ShortenLink>(m_repo, m_gen);
    m_resolve = std::make_shared<application::ResolveLink>(m_repo);
    m_delete = std::make_shared<application::DeleteLink>(m_repo);
    return *this;
}

AppBuilder& AppBuilder::httpClient() {
    astra::http2::ClientConfig client_config;
    if (m_config.bootstrap().has_dataservice() && 
        m_config.bootstrap().dataservice().has_client()) {
        client_config = m_config.bootstrap().dataservice().client();
    }
    m_http_client = std::make_unique<astra::http2::Http2Client>(client_config);
    return *this;
}

AppBuilder& AppBuilder::serviceResolver() {
    auto resolver = std::make_unique<astra::service_discovery::StaticServiceResolver>();
    resolver->register_service("dataservice", "localhost", 8080);
    m_resolver = std::move(resolver);
    return *this;
}

AppBuilder& AppBuilder::dataAdapter() {
    m_data_adapter = std::make_shared<service::HttpDataServiceAdapter>(
        *m_http_client, *m_resolver, "dataservice");
    return *this;
}

AppBuilder& AppBuilder::msgHandler() {
    m_msg_handler = std::make_unique<UriShortenerMessageHandler>(
        m_data_adapter, nullptr);
    return *this;
}

AppBuilder& AppBuilder::pool() {
    size_t workers = 4;
    if (m_config.bootstrap().has_execution() && 
        m_config.bootstrap().execution().has_shared_queue()) {
        workers = m_config.bootstrap().execution().shared_queue().num_workers();
    }
    
    m_obs_msg_handler = std::make_unique<ObservableMessageHandler>(*m_msg_handler);
    m_pool = std::make_unique<astra::execution::StickyQueue>(workers, *m_obs_msg_handler);
    
    auto response_queue = std::shared_ptr<astra::execution::IQueue>(
        m_pool.get(), [](auto*) {});
    m_msg_handler->setResponseQueue(response_queue);
    
    return *this;
}

AppBuilder& AppBuilder::reqHandler() {
    m_req_handler = std::make_unique<UriShortenerRequestHandler>(*m_pool);
    return *this;
}

AppBuilder& AppBuilder::wrapObservable() {
    m_obs_req_handler = std::make_unique<ObservableRequestHandler>(*m_req_handler);
    return *this;
}

AppBuilder& AppBuilder::loadShedder() {
    size_t max_concurrent = 1000;
    if (m_config.has_runtime() && m_config.runtime().has_load_shedder() &&
        m_config.runtime().load_shedder().max_concurrent_requests() > 0) {
        max_concurrent = m_config.runtime().load_shedder().max_concurrent_requests();
    }
    auto policy = astra::resilience::LoadShedderPolicy::create(max_concurrent, "uri_shortener");
    m_load_shedder = std::make_unique<astra::resilience::AtomicLoadShedder>(std::move(policy));
    return *this;
}

astra::outcome::Result<UriShortenerApp, AppError> AppBuilder::build() {
    const auto& bootstrap = m_config.bootstrap();
    std::string address = bootstrap.has_server() ? bootstrap.server().address() : "0.0.0.0";
    std::string port = bootstrap.has_server() ? std::to_string(bootstrap.server().port()) : "8080";
    
    if (address.empty()) {
        return astra::outcome::Result<UriShortenerApp, AppError>::Err(AppError::InvalidConfig);
    }
    if (port.empty() || port == "0") {
        return astra::outcome::Result<UriShortenerApp, AppError>::Err(AppError::InvalidConfig);
    }

    initObservability();
    
    m_server = std::make_unique<astra::http2::Http2Server>(bootstrap.server());
    m_pool->start();

    return astra::outcome::Result<UriShortenerApp, AppError>::Ok(UriShortenerApp(
        std::move(m_repo),
        std::move(m_gen),
        std::move(m_shorten),
        std::move(m_resolve),
        std::move(m_delete),
        std::move(m_http_client),
        std::move(m_resolver),
        std::move(m_data_adapter),
        std::move(m_msg_handler),
        std::move(m_obs_msg_handler),
        std::move(m_pool),
        std::move(m_req_handler),
        std::move(m_obs_req_handler),
        std::move(m_server),
        std::move(m_load_shedder)
    ));
}

void AppBuilder::initObservability() {
    const auto& bootstrap = m_config.bootstrap();
    
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
    
    obs::init(obs_config);
}

} // namespace uri_shortener
