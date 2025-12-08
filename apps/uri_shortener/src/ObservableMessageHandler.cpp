#include "ObservableMessageHandler.h"
#include <Message.h>
#include <chrono>
#include <any>

namespace url_shortener {

ObservableMessageHandler::ObservableMessageHandler(astra::execution::IMessageHandler& inner)
    : m_inner(inner)
    , m_messages_processed(obs::counter("uri_shortener.messages.processed", "Total messages processed"))
    , m_messages_failed(obs::counter("uri_shortener.messages.failed", "Total messages failed"))
    , m_processing_time(obs::histogram("uri_shortener.messages.duration_ms", "Message processing time in ms")) {
}

void ObservableMessageHandler::handle(astra::execution::Message& msg) {
    // Create span from trace context
    auto span = obs::span("uri_shortener.message.handle", msg.trace_ctx);
    
    // Add session ID attribute
    if (span) {
        span->attr("session_id", static_cast<int64_t>(msg.session_id));
    }
    
    // Try to add message type attribute
    try {
        auto& payload = std::any_cast<UriPayload&>(msg.payload);
        std::string msg_type = get_message_type(payload);
        if (span) {
            span->attr("message_type", msg_type);
        }
    } catch (const std::bad_any_cast&) {
        // Not a UriPayload, skip type attribute
    }
    
    auto start = std::chrono::steady_clock::now();
    
    try {
        // Delegate to inner handler
        m_inner.handle(msg);
        
        m_messages_processed.inc();
        if (span) {
            span->set_ok();
        }
    } catch (const std::exception& e) {
        m_messages_failed.inc();
        if (span) {
            span->set_error(e.what());
        }
        obs::error(std::string("Message handling failed: ") + e.what());
        throw;
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    m_processing_time.record(static_cast<double>(duration_ms));
}

std::string ObservableMessageHandler::get_message_type(const UriPayload& payload) {
    return std::visit(overloaded{
        [](const HttpRequestMsg&) { return std::string("http_request"); },
        [](const DbQueryMsg&) { return std::string("db_query"); },
        [](const DbResponseMsg&) { return std::string("db_response"); }
    }, payload);
}

} // namespace url_shortener
