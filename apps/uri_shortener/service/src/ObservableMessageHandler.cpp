#include "ObservableMessageHandler.h"

#include <Message.h>
#include <Provider.h>
#include <any>
#include <chrono>

namespace uri_shortener {

ObservableMessageHandler::ObservableMessageHandler(
    astra::execution::IMessageHandler &inner)
    : m_inner(inner),
      m_tracer(obs::Provider::instance().get_tracer("uri-shortener")) {
  m_metrics.counter("messages_processed", "uri_shortener.messages.processed")
      .counter("messages_failed", "uri_shortener.messages.failed")
      .duration_histogram("processing_time", "uri_shortener.messages.duration");
}

void ObservableMessageHandler::handle(astra::execution::Message &msg) {
  auto span =
      m_tracer->start_span("uri_shortener.message.handle", msg.trace_ctx);
  span->attr("affinity_key", static_cast<int64_t>(msg.affinity_key));

  auto start = std::chrono::steady_clock::now();

  try {
    m_inner.handle(msg);

    m_metrics.counter("messages_processed").inc();
    span->set_status(obs::StatusCode::Ok);
  } catch (const std::exception &e) {
    m_metrics.counter("messages_failed").inc();
    span->set_status(obs::StatusCode::Error, e.what());
    obs::error("Message handling failed", {{"error", e.what()}});
    span->end();
    throw;
  }

  auto duration = std::chrono::steady_clock::now() - start;
  m_metrics.duration_histogram("processing_time").record(duration);
  span->end();
}

} // namespace uri_shortener
