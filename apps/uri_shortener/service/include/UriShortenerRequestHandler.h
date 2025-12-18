#pragma once

#include <IMessageHandler.h>
#include <StickyQueue.h>
#include <IRequest.h>
#include <IResponse.h>
#include <Context.h>
#include <memory>

namespace url_shortener {

/**
 * @brief Request Handler for URI Shortener
 * 
 * Entry point for HTTP requests. Submits to pool for async processing.
 */
class UriShortenerRequestHandler {
public:
    explicit UriShortenerRequestHandler(astra::execution::StickyQueue& pool);
    
    /**
     * @brief Handle incoming HTTP request
     * 
     * Submits to pool for async processing.
     */
    void handle(std::shared_ptr<astra::router::IRequest> req, std::shared_ptr<astra::router::IResponse> res);

private:
    astra::execution::StickyQueue& m_pool;
    
    uint64_t generate_session_id(astra::router::IRequest& req);
};

} // namespace url_shortener

