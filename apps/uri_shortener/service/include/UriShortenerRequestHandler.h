#pragma once

#include <Context.h>
#include <IExecutor.h>
#include <IMessageHandler.h>
#include <IRequest.h>
#include <IResponse.h>
#include <memory>

namespace uri_shortener {

class UriShortenerRequestHandler {
public:
  explicit UriShortenerRequestHandler(astra::execution::IExecutor &executor);

  void handle(std::shared_ptr<astra::router::IRequest> req,
              std::shared_ptr<astra::router::IResponse> res);

private:
  astra::execution::IExecutor &m_executor;

  uint64_t generate_session_id(astra::router::IRequest &req);
};

} // namespace uri_shortener
