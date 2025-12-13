#include "UriShortenerRequestHandler.h"
#include "UriMessages.h"
#include "Http2Request.h"
#include "Http2Response.h"
#include <Message.h>
#include <functional>

namespace url_shortener {

UriShortenerRequestHandler::UriShortenerRequestHandler(astra::execution::StickyQueue& pool)
    : m_pool(pool) {
}

void UriShortenerRequestHandler::handle(router::IRequest& req, router::IResponse& res) {
    // Cast to concrete types (we know they are http2server::Request/Response)
    auto& http2_req = static_cast<http2server::Request&>(req);
    auto& http2_res = static_cast<http2server::Response&>(res);
    
    // Create message with copies of request/response handles
    HttpRequestMsg http_msg{http2_req, http2_res};
    
    // Generate session ID for affinity
    uint64_t session_id = generate_session_id(req);
    
    // Capture current trace context
    obs::Context trace_ctx = obs::Context::create();
    
    // Submit to pool
    astra::execution::Message msg{
        session_id,
        trace_ctx,
        UriPayload{std::move(http_msg)}
    };
    
    m_pool.submit(std::move(msg));
}

uint64_t UriShortenerRequestHandler::generate_session_id(router::IRequest& req) {
    // Use path + method hash for session affinity
    // This ensures same endpoint goes to same worker
    std::string key = std::string(req.method()) + ":" + std::string(req.path());
    return std::hash<std::string>{}(key);
}

} // namespace url_shortener
