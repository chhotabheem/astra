#pragma once

#include <Context.h>
#include <IMessageHandler.h>
#include <IRequest.h>
#include <IResponse.h>
#include <StickyQueue.h>
#include <memory>

namespace uri_shortener {

class UriShortenerRequestHandler {
public:
  explicit UriShortenerRequestHandler(astra::execution::StickyQueue &pool);

  void handle(std::shared_ptr<astra::router::IRequest> req,
              std::shared_ptr<astra::router::IResponse> res);

private:
  astra::execution::StickyQueue &m_pool;

  uint64_t generate_session_id(astra::router::IRequest &req);
};

} // namespace uri_shortener
