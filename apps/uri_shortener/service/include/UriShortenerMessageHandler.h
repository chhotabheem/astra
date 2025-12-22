#pragma once

#include <IMessageHandler.h>
#include <IQueue.h>
#include <IRequest.h>
#include <IResponse.h>
#include "IDataServiceAdapter.h"
#include "DataServiceMessages.h"
#include <Context.h>
#include <memory>

namespace uri_shortener {

class UriShortenerMessageHandler : public astra::execution::IMessageHandler {
public:
    UriShortenerMessageHandler(
        std::shared_ptr<service::IDataServiceAdapter> adapter,
        std::shared_ptr<astra::execution::IQueue> response_queue
    );
    
    void setResponseQueue(std::shared_ptr<astra::execution::IQueue> queue);
    
    void handle(astra::execution::Message& msg) override;

private:
    void processHttpRequest(
        std::shared_ptr<astra::router::IRequest> req,
        std::shared_ptr<astra::router::IResponse> res,
        uint64_t session_id, 
        obs::Context& trace_ctx
    );
    
    void processDataServiceResponse(service::DataServiceResponse& resp);
    
    std::string determine_operation(const std::string& method, const std::string& path);
    service::DataServiceOperation to_data_service_op(const std::string& operation);
    
    std::shared_ptr<service::IDataServiceAdapter> m_adapter;
    std::shared_ptr<astra::execution::IQueue> m_response_queue;
};

}




