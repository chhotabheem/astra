#include "UriShortenerApp.h"

#include "Http2Client.h"
#include "Http2Server.h"
#include "IServiceResolver.h"
#include "ObservableMessageHandler.h"
#include "ObservableRequestHandler.h"
#include "Router.h"
#include "UriShortenerMessageHandler.h"
#include "UriShortenerRequestHandler.h"

#include <AffinityExecutor.h>
#include <Http2Response.h>
#include <IRequest.h>
#include <IResponse.h>
#include <Log.h>
#include <Metrics.h>
#include <Provider.h>
#include <resilience/impl/AtomicLoadShedder.h>

namespace uri_shortener {

UriShortenerApp::UriShortenerApp(UriShortenerComponents components)
    : m_components(std::move(components)) {
}

UriShortenerApp::~UriShortenerApp() {
  if (m_components.executor) {
    m_components.executor->stop();
  }
}

UriShortenerApp::UriShortenerApp(UriShortenerApp &&) noexcept = default;
UriShortenerApp &
UriShortenerApp::operator=(UriShortenerApp &&) noexcept = default;

int UriShortenerApp::run() {
  auto accepted = obs::counter("load_shedder.accepted");
  auto rejected = obs::counter("load_shedder.rejected");

  auto resilient = [this, accepted,
                    rejected](std::shared_ptr<astra::router::IRequest> req,
                              std::shared_ptr<astra::router::IResponse> res) {
    auto guard = m_components.load_shedder->try_acquire();
    if (!guard) {
      rejected.inc();
      obs::warn("Load shedder rejected request",
                {{"current",
                  std::to_string(m_components.load_shedder->current_count())},
                 {"max", std::to_string(
                             m_components.load_shedder->max_concurrent())}});
      res->set_status(503);
      res->set_header("Content-Type", "application/json");
      res->set_header("Retry-After", "1");
      res->write(R"({"error": "Service overloaded"})");
      res->close();
      return;
    }

    accepted.inc();

    auto http_res = std::dynamic_pointer_cast<astra::http2::Http2Response>(res);
    if (http_res) {
      http_res->add_scoped_resource(
          std::make_unique<astra::resilience::LoadShedderGuard>(
              std::move(*guard)));
    }

    m_components.obs_req_handler->handle(req, res);
  };

  m_components.router->add(astra::router::HttpMethod::POST, "/shorten",
                           resilient);
  m_components.router->add(astra::router::HttpMethod::GET, "/:code", resilient);
  m_components.router->add(astra::router::HttpMethod::DELETE, "/:code",
                           resilient);

  m_components.router->add(astra::router::HttpMethod::GET, "/health",
                           [](std::shared_ptr<astra::router::IRequest>,
                              std::shared_ptr<astra::router::IResponse> res) {
                             res->set_status(200);
                             res->set_header("Content-Type",
                                             "application/json");
                             res->write(R"({"status": "ok"})");
                             res->close();
                           });

  obs::info("URI Shortener listening");
  obs::info("Using message-based architecture",
            {{"lanes", std::to_string(m_components.executor->lane_count())}});
  obs::info("Load shedder enabled",
            {{"max_concurrent",
              std::to_string(m_components.load_shedder->max_concurrent())}});

  auto start_result = m_components.server->start();
  if (!start_result) {
    obs::error("Failed to start server");
    return 1;
  }

  m_components.server->join();
  return 0;
}

} // namespace uri_shortener
