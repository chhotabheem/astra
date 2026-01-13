#pragma once

#include <memory>

namespace astra::router {
class Router;
}
namespace astra::http2 {
class Http2Server;
class Http2Client;
} // namespace astra::http2
namespace astra::service_discovery {
class IServiceResolver;
}
namespace astra::execution {
class AffinityExecutor;
}
namespace astra::resilience {
class AtomicLoadShedder;
}

namespace uri_shortener {

namespace domain {
class ILinkRepository;
class ICodeGenerator;
} // namespace domain
namespace application {
class ShortenLink;
class ResolveLink;
class DeleteLink;
} // namespace application
namespace service {
class IDataServiceAdapter;
}
class UriShortenerMessageHandler;
class ObservableMessageHandler;
class UriShortenerRequestHandler;
class ObservableRequestHandler;

struct UriShortenerComponents {
  std::shared_ptr<domain::ILinkRepository> repo;
  std::shared_ptr<domain::ICodeGenerator> gen;
  std::shared_ptr<application::ShortenLink> shorten;
  std::shared_ptr<application::ResolveLink> resolve;
  std::shared_ptr<application::DeleteLink> del;

  std::unique_ptr<astra::http2::Http2Client> http_client;
  std::unique_ptr<astra::service_discovery::IServiceResolver> resolver;
  std::shared_ptr<service::IDataServiceAdapter> data_adapter;

  std::unique_ptr<UriShortenerMessageHandler> msg_handler;
  std::unique_ptr<ObservableMessageHandler> obs_msg_handler;
  std::unique_ptr<astra::execution::AffinityExecutor> executor;
  std::unique_ptr<UriShortenerRequestHandler> req_handler;
  std::unique_ptr<ObservableRequestHandler> obs_req_handler;

  std::unique_ptr<astra::router::Router> router;
  std::unique_ptr<astra::http2::Http2Server> server;
  std::unique_ptr<astra::resilience::AtomicLoadShedder> load_shedder;

  UriShortenerComponents();
  ~UriShortenerComponents();
  UriShortenerComponents(UriShortenerComponents &&);
  UriShortenerComponents &operator=(UriShortenerComponents &&);
  UriShortenerComponents(const UriShortenerComponents &) = delete;
  UriShortenerComponents &operator=(const UriShortenerComponents &) = delete;
};

} // namespace uri_shortener
