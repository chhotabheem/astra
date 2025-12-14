#pragma once

#include <IMessageHandler.h>
#include <Span.h>
#include <Tracer.h>
#include <MetricsRegistry.h>
#include <Log.h>
#include <memory>

namespace url_shortener {

/**
 * @brief Observable decorator for message handler.
 * 
 * Adds observability (spans, metrics) to message handling.
 */
class ObservableMessageHandler : public astra::execution::IMessageHandler {
public:
    explicit ObservableMessageHandler(astra::execution::IMessageHandler& inner);
    
    void handle(astra::execution::Message& msg) override;

private:
    astra::execution::IMessageHandler& m_inner;
    std::shared_ptr<obs::Tracer> m_tracer;
    obs::MetricsRegistry m_metrics;
};

} // namespace url_shortener

