#include "DataServiceHandler.h"

#include <Log.h>
#include <Message.h>

namespace uri_shortener::service {

DataServiceHandler::DataServiceHandler(
    IDataServiceAdapter &adapter,
    astra::execution::IExecutor &response_executor)
    : m_adapter(adapter), m_response_executor(response_executor) {
}

void DataServiceHandler::handle(astra::execution::Message &msg) {
  // Extract the DataServiceRequest from the message
  auto &request = std::any_cast<DataServiceRequest &>(msg.payload);

  // Capture affinity_key and trace_ctx for the callback
  auto affinity_key = msg.affinity_key;
  auto trace_ctx = msg.trace_ctx;
  auto &response_executor = m_response_executor;

  // Call the adapter with a callback that submits the response back
  m_adapter.execute(std::move(request),
                    [affinity_key, trace_ctx,
                     &response_executor](DataServiceResponse response) {
                      // Create response message
                      astra::execution::Message response_msg;
                      response_msg.affinity_key = affinity_key;
                      response_msg.trace_ctx = trace_ctx;
                      response_msg.payload = std::move(response);

                      // Submit to executor for processing
                      response_executor.submit(std::move(response_msg));
                    });
}

} // namespace uri_shortener::service
