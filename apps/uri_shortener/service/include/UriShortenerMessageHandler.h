#pragma once

#include <IMessageHandler.h>
#include <IQueue.h>
#include <IRequest.h>
#include <IResponse.h>
#include "IDataServiceAdapter.h"
#include "DataServiceMessages.h"
#include <Context.h>
#include <memory>

namespace url_shortener {

/**
 * @brief Message Handler for URI Shortener
 * 
 * Handles request/response pairs from StickyQueue.
 * Calls IDataServiceAdapter for async backend calls.
 * Response callback submits back to response queue.
 */
class UriShortenerMessageHandler : public astra::execution::IMessageHandler {
public:
    /// @param adapter Data service adapter for backend calls
    /// @param response_queue Queue to submit responses back to (StickyQueue)
    UriShortenerMessageHandler(
        std::shared_ptr<service::IDataServiceAdapter> adapter,
        std::shared_ptr<astra::execution::IQueue> response_queue
    );
    
    /// Set response queue (for breaking circular dependency during wiring)
    void setResponseQueue(std::shared_ptr<astra::execution::IQueue> queue);
    
    void handle(astra::execution::Message& msg) override;

private:
    void processHttpRequest(
        std::shared_ptr<router::IRequest> req,
        std::shared_ptr<router::IResponse> res,
        uint64_t session_id, 
        obs::Context& trace_ctx
    );
    
    void processDataServiceResponse(service::DataServiceResponse& resp);
    
    std::string determine_operation(const std::string& method, const std::string& path);
    service::DataServiceOperation to_data_service_op(const std::string& operation);
    
    std::shared_ptr<service::IDataServiceAdapter> m_adapter;
    std::shared_ptr<astra::execution::IQueue> m_response_queue;
};

} // namespace url_shortener




