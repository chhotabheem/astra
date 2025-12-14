#pragma once

#include "IDataServiceAdapter.h"
#include <IMessageHandler.h>
#include <IQueue.h>
#include <memory>

namespace url_shortener::service {

/// Handler for DataServiceRequest messages on SharedQueue
/// Extracts requests, calls adapter, and submits responses back to StickyQueue
class DataServiceHandler : public astra::execution::IMessageHandler {
public:
    /// Construct with adapter and response queue
    /// @param adapter The data service adapter to use
    /// @param response_queue Queue to submit responses back to
    DataServiceHandler(
        IDataServiceAdapter& adapter,
        std::shared_ptr<astra::execution::IQueue> response_queue
    );
    
    ~DataServiceHandler() override = default;
    
    /// Handle a message from SharedQueue
    void handle(astra::execution::Message& msg) override;

private:
    IDataServiceAdapter& m_adapter;
    std::shared_ptr<astra::execution::IQueue> m_response_queue;
};

} // namespace url_shortener::service
