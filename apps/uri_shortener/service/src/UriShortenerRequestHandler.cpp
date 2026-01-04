#include "UriShortenerRequestHandler.h"

#include <Message.h>
#include <functional>
#include <utility>

namespace uri_shortener {

UriShortenerRequestHandler::UriShortenerRequestHandler(
    astra::execution::IExecutor &executor)
    : m_executor(executor) {
}

void UriShortenerRequestHandler::handle(
    std::shared_ptr<astra::router::IRequest> req,
    std::shared_ptr<astra::router::IResponse> res) {
  // Generate affinity key for session affinity
  uint64_t affinity_key = generate_session_id(*req);

  // Capture current trace context
  obs::Context trace_ctx = obs::Context::create();

  // Submit to executor with request/response pair as payload
  astra::execution::Message msg{affinity_key, trace_ctx,
                                std::make_pair(req, res)};

  m_executor.submit(std::move(msg));
}

uint64_t
UriShortenerRequestHandler::generate_session_id(astra::router::IRequest &req) {
  // Use path + method hash for session affinity
  std::string key = std::string(req.method()) + ":" + std::string(req.path());
  return std::hash<std::string>{}(key);
}

} // namespace uri_shortener
