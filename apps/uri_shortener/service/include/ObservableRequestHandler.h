#pragma once

#include "UriShortenerRequestHandler.h"
#include <Span.h>
#include <Tracer.h>
#include <MetricsRegistry.h>
#include <Log.h>
#include <memory>

namespace url_shortener {

/**
 * @brief Observable decorator for URI Shortener request handler.
 * 
 * Wraps the request handler and adds observability.
 */
class ObservableRequestHandler {
public:
    explicit ObservableRequestHandler(UriShortenerRequestHandler& inner);
    
    void handle(std::shared_ptr<router::IRequest> req, std::shared_ptr<router::IResponse> res);

private:
    UriShortenerRequestHandler& m_inner;
    std::shared_ptr<obs::Tracer> m_tracer;
    obs::MetricsRegistry m_metrics;
};

} // namespace url_shortener

