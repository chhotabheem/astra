#include "ObservableMessageHandler.h"
#include <Message.h>
#include <chrono>
#include <any>

namespace url_shortener {

ObservableMessageHandler::ObservableMessageHandler(astra::execution::IMessageHandler& inner)
    : m_inner(inner)
{
    m_metrics
        .counter("messages_processed", "uri_shortener.messages.processed")
        .counter("messages_failed", "uri_shortener.messages.failed")
        .duration_histogram("processing_time", "uri_shortener.messages.duration");
}

void ObservableMessageHandler::handle(astra::execution::Message& msg) {
    auto span = obs::span("uri_shortener.message.handle", msg.trace_ctx);
    span.attr("session_id", static_cast<int64_t>(msg.session_id));
    
    // Try to add message type attribute
    try {
        auto& payload = std::any_cast<UriPayload&>(msg.payload);
        std::string msg_type = get_message_type(payload);
        span.attr("message_type", msg_type);
    } catch (const std::bad_any_cast&) {
        // Not a UriPayload, skip type attribute
    }
    
    auto start = std::chrono::steady_clock::now();
    
    try {
        m_inner.handle(msg);
        
        m_metrics.counter("messages_processed").inc();
        span.set_status(obs::StatusCode::Ok);
    } catch (const std::exception& e) {
        m_metrics.counter("messages_failed").inc();
        span.set_status(obs::StatusCode::Error, e.what());
        obs::error("Message handling failed", {{"error", e.what()}});
        throw;
    }
    
    auto duration = std::chrono::steady_clock::now() - start;
    m_metrics.duration_histogram("processing_time").record(duration);
}

std::string ObservableMessageHandler::get_message_type(const UriPayload& payload) {
    return std::visit(overloaded{
        [](const HttpRequestMsg&) { return std::string("http_request"); },
        [](const DbQueryMsg&) { return std::string("db_query"); },
        [](const DbResponseMsg&) { return std::string("db_response"); }
    }, payload);
}

} // namespace url_shortener
