#pragma once

#include <IMessageHandler.h>
#include "UriMessages.h"
#include <obs/Span.h>
#include <obs/Metrics.h>
#include <obs/Log.h>
#include <memory>

namespace url_shortener {

/**
 * @brief Observable decorator for URI Shortener message handler.
 * 
 * Wraps a message handler and adds:
 * - Span creation from trace context
 * - Message type attributes
 * - Processing metrics
 * - Error logging
 */
class ObservableMessageHandler : public astra::execution::IMessageHandler {
public:
    explicit ObservableMessageHandler(astra::execution::IMessageHandler& inner);
    
    void handle(astra::execution::Message& msg) override;

private:
    astra::execution::IMessageHandler& m_inner;
    
    obs::Counter& m_messages_processed;
    obs::Counter& m_messages_failed;
    obs::Histogram& m_processing_time;
    
    std::string get_message_type(const UriPayload& payload);
};

} // namespace url_shortener
