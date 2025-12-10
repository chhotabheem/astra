#include <Span.h>
#include "ProviderImpl.h"
#include <Provider.h>

#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/span.h>
#include <cstring>

namespace obs {

namespace trace_api = opentelemetry::trace;
namespace nostd = opentelemetry::nostd;

// Pimpl - hide OTel SDK types
struct Span::Impl {
    nostd::shared_ptr<trace_api::Span> otel_span;
    Context span_context;
    
    Impl(nostd::shared_ptr<trace_api::Span> span, const Context& ctx)
        : otel_span(std::move(span)), span_context(ctx) {}
    
    ~Impl() {
        if (otel_span) {
            otel_span->End();
            
            // Pop from active span stack
            auto& provider = Provider::instance();
            provider.impl().pop_active_span();
        }
    }
};

// Constructor
Span::Span(Impl* impl) : m_impl(impl) {}

// Destructor
Span::~Span() = default;

// Move constructor
Span::Span(Span&&) noexcept = default;

// Move assignment
Span& Span::operator=(Span&&) noexcept = default;

// Attributes
Span& Span::attr(std::string_view key, std::string_view value) {
    if (m_impl && m_impl->otel_span) {
        m_impl->otel_span->SetAttribute(std::string(key), std::string(value));
    }
    return *this;
}

Span& Span::attr(std::string_view key, int64_t value) {
    if (m_impl && m_impl->otel_span) {
        m_impl->otel_span->SetAttribute(std::string(key), value);
    }
    return *this;
}

Span& Span::attr(std::string_view key, double value) {
    if (m_impl && m_impl->otel_span) {
        m_impl->otel_span->SetAttribute(std::string(key), value);
    }
    return *this;
}

Span& Span::attr(std::string_view key, bool value) {
    if (m_impl && m_impl->otel_span) {
        m_impl->otel_span->SetAttribute(std::string(key), value);
    }
    return *this;
}

// Status
Span& Span::set_status(StatusCode code, std::string_view message) {
    if (m_impl && m_impl->otel_span) {
        trace_api::StatusCode otel_code;
        switch (code) {
            case StatusCode::Unset:
                otel_code = trace_api::StatusCode::kUnset;
                break;
            case StatusCode::Ok:
                otel_code = trace_api::StatusCode::kOk;
                break;
            case StatusCode::Error:
                otel_code = trace_api::StatusCode::kError;
                break;
            default:
                otel_code = trace_api::StatusCode::kUnset;
        }
        m_impl->otel_span->SetStatus(otel_code, std::string(message));
    }
    return *this;
}

// Kind
Span& Span::kind(SpanKind kind) {
    if (m_impl && m_impl->otel_span) {
        const char* kind_str = "internal";
        switch (kind) {
            case SpanKind::Internal: kind_str = "internal"; break;
            case SpanKind::Server: kind_str = "server"; break;
            case SpanKind::Client: kind_str = "client"; break;
            case SpanKind::Producer: kind_str = "producer"; break;
            case SpanKind::Consumer: kind_str = "consumer"; break;
        }
        m_impl->otel_span->SetAttribute("span.kind", kind_str);
    }
    return *this;
}

// Events
Span& Span::add_event(std::string_view name) {
    if (m_impl && m_impl->otel_span) {
        m_impl->otel_span->AddEvent(std::string(name));
    }
    return *this;
}

Span& Span::add_event(std::string_view name, Attributes attrs) {
    if (m_impl && m_impl->otel_span) {
        // Store strings to ensure they outlive the OTel call
        std::vector<std::pair<std::string, std::string>> stored_attrs;
        stored_attrs.reserve(attrs.size());
        for (const auto& attr : attrs) {
            stored_attrs.emplace_back(std::string(attr.first), std::string(attr.second));
        }
        
        // Build OTel attributes map using stored strings
        std::map<std::string, opentelemetry::common::AttributeValue> otel_attrs;
        for (const auto& attr : stored_attrs) {
            otel_attrs[attr.first] = attr.second;
        }
        m_impl->otel_span->AddEvent(std::string(name), otel_attrs);
    }
    return *this;
}

// Context extraction
Context Span::context() const {
    if (!m_impl) {
        return Context{};
    }
    return m_impl->span_context;
}

// Recording check
bool Span::is_recording() const {
    if (m_impl && m_impl->otel_span) {
        return m_impl->otel_span->IsRecording();
    }
    return false;
}

// Span creation functions
Span span(std::string_view name) {
    auto& provider = Provider::instance();
    auto& impl = provider.impl();
    
    auto tracer = impl.get_tracer();
    if (!tracer) {
        return Span{nullptr};
    }
    
    auto parent_ctx = impl.get_active_context();
    
    trace_api::StartSpanOptions options;
    if (parent_ctx.is_valid()) {
        options.parent = impl.context_to_otel(parent_ctx);
    }
    
    auto otel_span = tracer->StartSpan(std::string(name), options);
    
    // Create new context with proper span_id
    Context ctx;
    if (parent_ctx.is_valid()) {
        // Child span - inherit trace_id from parent
        ctx = parent_ctx;
        ctx.span_id.value = impl.generate_span_id();
    } else {
        // Root span - new trace_id AND span_id
        ctx = Context::create();
        ctx.span_id.value = impl.generate_span_id();  // FIX: Generate span_id for root span
    }
    
    Span span{new Span::Impl{otel_span, ctx}};
    impl.push_active_span(ctx);
    
    return span;
}

Span span(std::string_view name, const Context& parent) {
    auto& provider = Provider::instance();
    auto& impl = provider.impl();
    
    auto tracer = impl.get_tracer();
    if (!tracer) {
        return Span{nullptr};
    }
    
    trace_api::StartSpanOptions options;
    if (parent.is_valid()) {
        options.parent = impl.context_to_otel(parent);
    }
    
    auto otel_span = tracer->StartSpan(std::string(name), options);
    
    Context ctx = parent.child(SpanId{impl.generate_span_id()});
    
    Span span{new Span::Impl{otel_span, ctx}};
    impl.push_active_span(ctx);
    
    return span;
}

} // namespace obs
