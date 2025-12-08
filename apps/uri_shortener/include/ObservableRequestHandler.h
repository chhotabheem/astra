#pragma once

#include "UriShortenerRequestHandler.h"
#include <obs/Span.h>
#include <obs/Metrics.h>
#include <obs/Log.h>

namespace url_shortener {

/**
 * @brief Observable decorator for URI Shortener request handler.
 * 
 * Wraps the request handler and adds:
 * - Root span creation for incoming HTTP requests
 * - Request metrics (count, latency)
 * - Error logging
 */
class ObservableRequestHandler {
public:
    explicit ObservableRequestHandler(UriShortenerRequestHandler& inner);
    
    void handle(router::IRequest& req, router::IResponse& res);

private:
    UriShortenerRequestHandler& m_inner;
    
    obs::Counter& m_requests_total;
    obs::Histogram& m_request_latency;
};

} // namespace url_shortener
