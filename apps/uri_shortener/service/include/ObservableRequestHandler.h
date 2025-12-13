#pragma once

#include "UriShortenerRequestHandler.h"
#include <Span.h>
#include <MetricsRegistry.h>
#include <Log.h>

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
    obs::MetricsRegistry m_metrics;
};

} // namespace url_shortener
