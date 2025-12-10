#pragma once

#include <IMessageHandler.h>
#include "UriMessages.h"
#include <Span.h>
#include <MetricsRegistry.h>
#include <Log.h>
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
    obs::MetricsRegistry m_metrics;
    
    std::string get_message_type(const UriPayload& payload);
};

} // namespace url_shortener
