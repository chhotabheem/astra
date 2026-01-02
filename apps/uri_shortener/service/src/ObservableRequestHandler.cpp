#include "ObservableRequestHandler.h"

#include <Provider.h>
#include <chrono>

namespace uri_shortener {

ObservableRequestHandler::ObservableRequestHandler(
    UriShortenerRequestHandler &inner)
    : m_inner(inner),
      m_tracer(obs::Provider::instance().get_tracer("uri-shortener")) {
  m_metrics.counter("requests_total", "uri_shortener.requests.total")
      .duration_histogram("request_latency", "uri_shortener.request.latency");
}

void ObservableRequestHandler::handle(
    std::shared_ptr<astra::router::IRequest> req,
    std::shared_ptr<astra::router::IResponse> res) {
  auto span = m_tracer->start_span("uri_shortener.http.request");
  span->kind(obs::SpanKind::Server);
  span->attr("http.method", req->method());
  span->attr("http.path", req->path());

  m_metrics.counter("requests_total").inc();
  auto start = std::chrono::steady_clock::now();

  try {
    m_inner.handle(req, res);
    span->set_status(obs::StatusCode::Ok);
  } catch (const std::exception &e) {
    span->set_status(obs::StatusCode::Error, e.what());
    obs::error("Request handling failed", {{"error", e.what()}});
    span->end();
    throw;
  }

  auto duration = std::chrono::steady_clock::now() - start;
  m_metrics.duration_histogram("request_latency").record(duration);
  span->end();
}

} // namespace uri_shortener
