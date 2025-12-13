#pragma once

#include <IMessageHandler.h>
#include <StickyQueue.h>
#include <IRequest.h>
#include <IResponse.h>
#include <Context.h>

namespace url_shortener {

/**
 * @brief Request Handler for URI Shortener
 * 
 * Entry point for HTTP requests. Creates HttpRequestMsg and submits to pool.
 * This replaces direct handler registration with router.
 */
class UriShortenerRequestHandler {
public:
    explicit UriShortenerRequestHandler(astra::execution::StickyQueue& pool);
    
    /**
     * @brief Handle incoming HTTP request
     * 
     * Creates HttpRequestMsg with copies of request/response,
     * then submits to pool for async processing.
     */
    void handle(router::IRequest& req, router::IResponse& res);

private:
    astra::execution::StickyQueue& m_pool;
    
    /**
     * @brief Generate session ID from request
     * 
     * Uses request path hash for session affinity.
     */
    uint64_t generate_session_id(router::IRequest& req);
};

} // namespace url_shortener
