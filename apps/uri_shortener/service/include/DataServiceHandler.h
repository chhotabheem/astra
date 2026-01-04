#pragma once

#include "IDataServiceAdapter.h"

#include <IExecutor.h>
#include <IMessageHandler.h>
#include <memory>

namespace uri_shortener::service {

/// Handler for DataServiceRequest messages
/// Extracts requests, calls adapter, and submits responses back to executor
class DataServiceHandler : public astra::execution::IMessageHandler {
public:
  /// Construct with adapter and response executor
  /// @param adapter The data service adapter to use
  /// @param response_executor Executor to submit responses back to
  DataServiceHandler(IDataServiceAdapter &adapter,
                     astra::execution::IExecutor &response_executor);

  ~DataServiceHandler() override = default;

  /// Handle a message
  void handle(astra::execution::Message &msg) override;

private:
  IDataServiceAdapter &m_adapter;
  astra::execution::IExecutor &m_response_executor;
};

} // namespace uri_shortener::service
