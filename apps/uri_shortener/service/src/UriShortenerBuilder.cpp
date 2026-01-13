#include "UriShortenerBuilder.h"

#include "DeleteLink.h"
#include "Http2Client.h"
#include "Http2Server.h"
#include "HttpDataServiceAdapter.h"
#include "InMemoryLinkRepository.h"
#include "ObservableLinkRepository.h"
#include "ObservableMessageHandler.h"
#include "ObservableRequestHandler.h"
#include "ProtoConfigLoader.h"
#include "RandomCodeGenerator.h"
#include "ResolveLink.h"
#include "Router.h"
#include "ShortenLink.h"
#include "StaticServiceResolver.h"
#include "UriShortenerApp.h"
#include "UriShortenerMessageHandler.h"
#include "UriShortenerRequestHandler.h"

#include <AffinityExecutor.h>
#include <Log.h>
#include <Provider.h>
#include <resilience/impl/AtomicLoadShedder.h>
#include <resilience/policy/LoadShedderPolicy.h>

namespace uri_shortener {

astra::outcome::Result<UriShortenerApp, BuilderError>
UriShortenerBuilder::bootstrap() {
  ::observability::Config bootstrap_obs;
  bootstrap_obs.set_service_name("uri-shortener");
  bootstrap_obs.set_service_version("1.0.0");
  bootstrap_obs.set_environment("bootstrap");
  obs::init(bootstrap_obs);

  auto load_result = ProtoConfigLoader::load();
  if (load_result.is_err()) {
    obs::error("Failed to load config", {{"error", load_result.error()}});
    return astra::outcome::Result<UriShortenerApp, BuilderError>::Err(
        BuilderError::InvalidConfig);
  }

  return UriShortenerBuilder(load_result.value())
      .domain()
      .backend()
      .messaging()
      .resilience()
      .build();
}

UriShortenerBuilder::UriShortenerBuilder(const Config &config)
    : m_config(config) {
}

UriShortenerBuilder &UriShortenerBuilder::domain() {
  return repo().codeGen().useCases();
}

UriShortenerBuilder &UriShortenerBuilder::backend() {
  return httpClient().serviceResolver().dataAdapter();
}

UriShortenerBuilder &UriShortenerBuilder::messaging() {
  return msgHandler().executor().reqHandler().wrapObservable();
}

UriShortenerBuilder &UriShortenerBuilder::resilience() {
  return loadShedder();
}

UriShortenerBuilder &UriShortenerBuilder::repo() {
  m_components.repo =
      std::make_shared<infrastructure::ObservableLinkRepository>(
          std::make_shared<infrastructure::InMemoryLinkRepository>());
  return *this;
}

UriShortenerBuilder &UriShortenerBuilder::codeGen() {
  m_components.gen = std::make_shared<infrastructure::RandomCodeGenerator>();
  return *this;
}

UriShortenerBuilder &UriShortenerBuilder::useCases() {
  m_components.shorten = std::make_shared<application::ShortenLink>(
      m_components.repo, m_components.gen);
  m_components.resolve =
      std::make_shared<application::ResolveLink>(m_components.repo);
  m_components.del =
      std::make_shared<application::DeleteLink>(m_components.repo);
  return *this;
}

UriShortenerBuilder &UriShortenerBuilder::httpClient() {
  ::http2::ClientConfig client_config;
  if (m_config.bootstrap().has_dataservice() &&
      m_config.bootstrap().dataservice().has_client()) {
    client_config = m_config.bootstrap().dataservice().client();
  }
  m_components.http_client =
      std::make_unique<astra::http2::Http2Client>(client_config);
  return *this;
}

UriShortenerBuilder &UriShortenerBuilder::serviceResolver() {
  auto resolver =
      std::make_unique<astra::service_discovery::StaticServiceResolver>();
  resolver->register_service("dataservice", "localhost", 8080);
  m_components.resolver = std::move(resolver);
  return *this;
}

UriShortenerBuilder &UriShortenerBuilder::dataAdapter() {
  m_components.data_adapter = std::make_shared<service::HttpDataServiceAdapter>(
      *m_components.http_client, *m_components.resolver, "dataservice");
  return *this;
}

UriShortenerBuilder &UriShortenerBuilder::msgHandler() {
  m_components.msg_handler =
      std::make_unique<UriShortenerMessageHandler>(m_components.data_adapter);
  return *this;
}

UriShortenerBuilder &UriShortenerBuilder::executor() {
  size_t num_lanes = 4;
  if (m_config.bootstrap().has_execution() &&
      m_config.bootstrap().execution().has_pool_executor()) {
    num_lanes = m_config.bootstrap().execution().pool_executor().num_workers();
  }

  m_components.obs_msg_handler =
      std::make_unique<ObservableMessageHandler>(*m_components.msg_handler);
  m_components.executor = std::make_unique<astra::execution::AffinityExecutor>(
      num_lanes, *m_components.obs_msg_handler);

  m_components.msg_handler->setResponseExecutor(*m_components.executor);

  return *this;
}

UriShortenerBuilder &UriShortenerBuilder::reqHandler() {
  m_components.req_handler =
      std::make_unique<UriShortenerRequestHandler>(*m_components.executor);
  return *this;
}

UriShortenerBuilder &UriShortenerBuilder::wrapObservable() {
  m_components.obs_req_handler =
      std::make_unique<ObservableRequestHandler>(*m_components.req_handler);
  return *this;
}

UriShortenerBuilder &UriShortenerBuilder::loadShedder() {
  size_t max_concurrent = 1000;
  if (m_config.has_runtime() && m_config.runtime().has_load_shedder() &&
      m_config.runtime().load_shedder().max_concurrent_requests() > 0) {
    max_concurrent =
        m_config.runtime().load_shedder().max_concurrent_requests();
  }
  auto policy = astra::resilience::LoadShedderPolicy::create(max_concurrent,
                                                             "uri_shortener");
  m_components.load_shedder =
      std::make_unique<astra::resilience::AtomicLoadShedder>(std::move(policy));
  return *this;
}

astra::outcome::Result<UriShortenerApp, BuilderError>
UriShortenerBuilder::build() {
  const auto &bootstrap = m_config.bootstrap();
  std::string uri =
      bootstrap.has_server() ? bootstrap.server().uri() : "0.0.0.0:8080";

  if (uri.empty()) {
    return astra::outcome::Result<UriShortenerApp, BuilderError>::Err(
        BuilderError::InvalidConfig);
  }

  initObservability();

  m_components.router = std::make_unique<astra::router::Router>();
  m_components.server = std::make_unique<astra::http2::Http2Server>(
      bootstrap.server(), *m_components.router);
  m_components.executor->start();

  return astra::outcome::Result<UriShortenerApp, BuilderError>::Ok(
      UriShortenerApp(std::move(m_components)));
}

void UriShortenerBuilder::initObservability() {
  const auto &bootstrap = m_config.bootstrap();

  ::observability::Config obs_config;
  if (bootstrap.has_service()) {
    obs_config.set_service_name(bootstrap.service().name());
    obs_config.set_environment(bootstrap.service().environment());
  } else {
    obs_config.set_service_name("uri_shortener");
    obs_config.set_environment("development");
  }

  if (bootstrap.has_observability()) {
    const auto &obs_proto = bootstrap.observability();
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
