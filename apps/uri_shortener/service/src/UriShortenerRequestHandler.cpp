#include "UriShortenerRequestHandler.h"
#include <Message.h>
#include <functional>
#include <utility>

namespace url_shortener {

UriShortenerRequestHandler::UriShortenerRequestHandler(astra::execution::StickyQueue& pool)
    : m_pool(pool) {
}

void UriShortenerRequestHandler::handle(
    std::shared_ptr<router::IRequest> req, 
    std::shared_ptr<router::IResponse> res
) {
    // Generate session ID for affinity
    uint64_t session_id = generate_session_id(*req);
    
    // Capture current trace context
    obs::Context trace_ctx = obs::Context::create();
    
    // Submit to pool with request/response pair as payload
    astra::execution::Message msg{
        session_id,
        trace_ctx,
        std::make_pair(req, res)
    };
    
    m_pool.submit(std::move(msg));
}

uint64_t UriShortenerRequestHandler::generate_session_id(router::IRequest& req) {
    // Use path + method hash for session affinity
    std::string key = std::string(req.method()) + ":" + std::string(req.path());
    return std::hash<std::string>{}(key);
}

} // namespace url_shortener

