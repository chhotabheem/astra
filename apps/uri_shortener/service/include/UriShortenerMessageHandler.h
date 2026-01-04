#pragma once

#include "DataServiceMessages.h"
#include "IDataServiceAdapter.h"

#include <Context.h>
#include <IExecutor.h>
#include <IMessageHandler.h>
#include <IRequest.h>
#include <IResponse.h>
#include <memory>

namespace uri_shortener {

class UriShortenerMessageHandler : public astra::execution::IMessageHandler {
public:
  explicit UriShortenerMessageHandler(
      std::shared_ptr<service::IDataServiceAdapter> adapter);

  void setResponseExecutor(astra::execution::IExecutor &executor);

  void handle(astra::execution::Message &msg) override;

private:
  void processHttpRequest(std::shared_ptr<astra::router::IRequest> req,
                          std::shared_ptr<astra::router::IResponse> res,
                          uint64_t affinity_key, obs::Context &trace_ctx);

  void processDataServiceResponse(service::DataServiceResponse &resp);

  std::string determine_operation(const std::string &method,
                                  const std::string &path);
  service::DataServiceOperation
  to_data_service_op(const std::string &operation);

  std::shared_ptr<service::IDataServiceAdapter> m_adapter;
  astra::execution::IExecutor *m_response_executor{nullptr};
};

} // namespace uri_shortener
