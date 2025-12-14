#include "TracerImpl.h"
#include "ProviderImpl.h"
#include "SpanImpl.h"
#include <Span.h>
#include <Log.h>

#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/span.h>

namespace astra::observability {

TracerImpl::TracerImpl(std::string name, ProviderImpl& provider)
    : m_name(std::move(name))
    , m_provider(provider) {}

std::shared_ptr<Span> TracerImpl::start_span(std::string_view name) {
    auto parent_ctx = m_provider.get_active_context();
    return start_span(name, parent_ctx);
}

std::shared_ptr<Span> TracerImpl::start_span(std::string_view name, const Context& parent) {
    auto tracer = m_provider.get_tracer();
    if (!tracer) {
        astra::observability::warn("Tracer not initialized - returning null span");
        return std::shared_ptr<Span>(new Span(nullptr));
    }
    
    opentelemetry::trace::StartSpanOptions options;
    if (parent.is_valid()) {
        options.parent = m_provider.context_to_otel(parent);
    }
    
    auto otel_span = tracer->StartSpan(std::string(name), options);
    
    Context ctx;
    if (parent.is_valid()) {
        ctx = parent.child(SpanId{m_provider.generate_span_id()});
    } else {
        ctx = Context::create();
        ctx.span_id.value = m_provider.generate_span_id();
    }
    
    auto span = std::shared_ptr<Span>(new Span(new Span::Impl{otel_span, ctx}));
    m_provider.push_active_span(ctx);
    
    return span;
}

} // namespace astra::observability

