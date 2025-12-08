#include "ObservableRequestHandler.h"
#include <chrono>

namespace url_shortener {

ObservableRequestHandler::ObservableRequestHandler(UriShortenerRequestHandler& inner)
    : m_inner(inner)
    , m_requests_total(obs::counter("uri_shortener.requests.total", "Total HTTP requests"))
    , m_request_latency(obs::histogram("uri_shortener.requests.latency_ms", "Request latency in ms")) {
}

void ObservableRequestHandler::handle(router::IRequest& req, router::IResponse& res) {
    // Create root span for HTTP request
    auto span = obs::span("uri_shortener.http.request");
    
    if (span) {
        span->attr("http.method", req.method());
        span->attr("http.path", req.path());
    }
    
    m_requests_total.inc();
    auto start = std::chrono::steady_clock::now();
    
    try {
        // Delegate to inner handler
        m_inner.handle(req, res);
        
        if (span) {
            span->set_ok();
        }
    } catch (const std::exception& e) {
        if (span) {
            span->set_error(e.what());
        }
        obs::error(std::string("Request handling failed: ") + e.what());
        throw;
    }
    
    auto end = std::chrono::steady_clock::now();
    auto latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    m_request_latency.record(static_cast<double>(latency_ms));
}

} // namespace url_shortener
