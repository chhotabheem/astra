#include "ObservableRequestHandler.h"
#include <chrono>

namespace url_shortener {

ObservableRequestHandler::ObservableRequestHandler(UriShortenerRequestHandler& inner)
    : m_inner(inner)
{
    m_metrics
        .counter("requests_total", "uri_shortener.requests.total")
        .duration_histogram("request_latency", "uri_shortener.request.latency");
}

void ObservableRequestHandler::handle(router::IRequest& req, router::IResponse& res) {
    auto span = obs::span("uri_shortener.http.request");
    span.kind(obs::SpanKind::Server);
    span.attr("http.method", req.method());
    span.attr("http.path", req.path());
    
    m_metrics.counter("requests_total").inc();
    auto start = std::chrono::steady_clock::now();
    
    try {
        m_inner.handle(req, res);
        span.set_status(obs::StatusCode::Ok);
    } catch (const std::exception& e) {
        span.set_status(obs::StatusCode::Error, e.what());
        obs::error("Request handling failed", {{"error", e.what()}});
        throw;
    }
    
    auto duration = std::chrono::steady_clock::now() - start;
    m_metrics.duration_histogram("request_latency").record(duration);
}

} // namespace url_shortener
