#include "DataServiceHandler.h"
#include <Message.h>
#include <Log.h>

namespace url_shortener::service {

DataServiceHandler::DataServiceHandler(
    IDataServiceAdapter& adapter,
    std::shared_ptr<astra::execution::IQueue> response_queue
) : m_adapter(adapter), m_response_queue(std::move(response_queue)) {}

void DataServiceHandler::handle(astra::execution::Message& msg) {
    // Extract the DataServiceRequest from the message
    auto& request = std::any_cast<DataServiceRequest&>(msg.payload);
    
    // Capture session_id and trace_ctx for the callback
    auto session_id = msg.session_id;
    auto trace_ctx = msg.trace_ctx;
    auto response_queue = m_response_queue;
    
    // Call the adapter with a callback that submits the response back to StickyQueue
    m_adapter.execute(std::move(request), 
        [session_id, trace_ctx, response_queue](DataServiceResponse response) {
            // Create response message
            astra::execution::Message response_msg;
            response_msg.session_id = session_id;
            response_msg.trace_ctx = trace_ctx;
            response_msg.payload = std::move(response);
            
            // Submit to StickyQueue for processing
            response_queue->submit(std::move(response_msg));
        }
    );
}

} // namespace url_shortener::service
