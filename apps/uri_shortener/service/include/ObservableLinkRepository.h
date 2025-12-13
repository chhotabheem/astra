/// @file ObservableLinkRepository.h
/// @brief Decorator that adds observability to any ILinkRepository

#pragma once

#include "ILinkRepository.h"
#include <MetricsRegistry.h>
#include <Span.h>
#include <Log.h>
#include <chrono>
#include <memory>

namespace url_shortener::infrastructure {

/**
 * @brief Observability decorator for ILinkRepository
 * 
 * Wraps any ILinkRepository implementation and adds:
 * - Timing histograms for each operation
 * - Success/error counters
 * - Distributed tracing spans
 * - Structured logging
 */
class ObservableLinkRepository : public domain::ILinkRepository {
public:
    explicit ObservableLinkRepository(std::shared_ptr<domain::ILinkRepository> inner)
        : m_inner(std::move(inner))
    {
        // Register all metrics using MetricsRegistry pattern
        m_metrics
            .duration_histogram("save", "link_repo.save.duration")
            .duration_histogram("find", "link_repo.find.duration")
            .duration_histogram("remove", "link_repo.remove.duration")
            .counter("save_success", "link_repo.save.success")
            .counter("save_error", "link_repo.save.error")
            .counter("find_success", "link_repo.find.success")
            .counter("find_miss", "link_repo.find.miss")
            .counter("remove_success", "link_repo.remove.success")
            .counter("remove_error", "link_repo.remove.error");
    }

    astra::Result<void, domain::DomainError> save(const domain::ShortLink& link) override {
        auto span = obs::span("LinkRepository.save");
        span.attr("short_code", std::string(link.code().value()));
        
        auto start = std::chrono::steady_clock::now();
        auto result = m_inner->save(link);
        auto duration = std::chrono::steady_clock::now() - start;
        
        m_metrics.duration_histogram("save").record(duration);
        
        if (result.is_ok()) {
            m_metrics.counter("save_success").inc();
            span.set_status(obs::StatusCode::Ok);
            obs::debug("Link saved", {{"code", link.code().value()}});
        } else {
            m_metrics.counter("save_error").inc();
            span.set_status(obs::StatusCode::Error, "save failed");
            obs::warn("Save failed", {{"code", link.code().value()}});
        }
        
        return result;
    }

    astra::Result<void, domain::DomainError> remove(const domain::ShortCode& code) override {
        auto span = obs::span("LinkRepository.remove");
        span.attr("short_code", std::string(code.value()));
        
        auto start = std::chrono::steady_clock::now();
        auto result = m_inner->remove(code);
        auto duration = std::chrono::steady_clock::now() - start;
        
        m_metrics.duration_histogram("remove").record(duration);
        
        if (result.is_ok()) {
            m_metrics.counter("remove_success").inc();
            span.set_status(obs::StatusCode::Ok);
        } else {
            m_metrics.counter("remove_error").inc();
            span.set_status(obs::StatusCode::Error, "remove failed");
        }
        
        return result;
    }

    astra::Result<domain::ShortLink, domain::DomainError> find_by_code(const domain::ShortCode& code) override {
        auto span = obs::span("LinkRepository.find_by_code");
        span.attr("short_code", std::string(code.value()));
        
        auto start = std::chrono::steady_clock::now();
        auto result = m_inner->find_by_code(code);
        auto duration = std::chrono::steady_clock::now() - start;
        
        m_metrics.duration_histogram("find").record(duration);
        
        if (result.is_ok()) {
            m_metrics.counter("find_success").inc();
            span.set_status(obs::StatusCode::Ok);
        } else {
            m_metrics.counter("find_miss").inc();
            span.attr("found", "false");
        }
        
        return result;
    }

    bool exists(const domain::ShortCode& code) override {
        return m_inner->exists(code);
    }

private:
    std::shared_ptr<domain::ILinkRepository> m_inner;
    obs::MetricsRegistry m_metrics;  // ONE member variable for ALL metrics
};

} // namespace url_shortener::infrastructure
