/// @file ObservableLinkRepository.h
/// @brief Decorator that adds observability to any ILinkRepository

#pragma once

#include "domain/ports/ILinkRepository.h"
#include <obs/Metrics.h>
#include <obs/Span.h>
#include <obs/Log.h>
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
        , m_save_histogram(obs::histogram("link_repo.save.duration_ms", "Time to save a link"))
        , m_find_histogram(obs::histogram("link_repo.find.duration_ms", "Time to find a link"))
        , m_remove_histogram(obs::histogram("link_repo.remove.duration_ms", "Time to remove a link"))
        , m_save_success(obs::counter("link_repo.save.success", "Successful saves"))
        , m_save_error(obs::counter("link_repo.save.error", "Failed saves"))
        , m_find_success(obs::counter("link_repo.find.success", "Successful finds"))
        , m_find_miss(obs::counter("link_repo.find.miss", "Find misses"))
        , m_remove_success(obs::counter("link_repo.remove.success", "Successful removes"))
        , m_remove_error(obs::counter("link_repo.remove.error", "Failed removes")) {}

    astra::Result<void, domain::DomainError> save(const domain::ShortLink& link) override {
        auto span = obs::span("LinkRepository.save");
        span->attr("short_code", std::string(link.code().value()));
        
        auto start = std::chrono::steady_clock::now();
        auto result = m_inner->save(link);
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        
        m_save_histogram.record(static_cast<double>(elapsed));
        
        if (result.is_ok()) {
            m_save_success.inc();
            span->set_ok();
            obs::debug("Link saved: " + std::string(link.code().value()));
        } else {
            m_save_error.inc();
            span->set_error("save failed");
            obs::warn("Save failed for: " + std::string(link.code().value()));
        }
        
        return result;
    }

    astra::Result<void, domain::DomainError> remove(const domain::ShortCode& code) override {
        auto span = obs::span("LinkRepository.remove");
        span->attr("short_code", std::string(code.value()));
        
        auto start = std::chrono::steady_clock::now();
        auto result = m_inner->remove(code);
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        
        m_remove_histogram.record(static_cast<double>(elapsed));
        
        if (result.is_ok()) {
            m_remove_success.inc();
            span->set_ok();
        } else {
            m_remove_error.inc();
            span->set_error("remove failed");
        }
        
        return result;
    }

    astra::Result<domain::ShortLink, domain::DomainError> find_by_code(const domain::ShortCode& code) override {
        auto span = obs::span("LinkRepository.find_by_code");
        span->attr("short_code", std::string(code.value()));
        
        auto start = std::chrono::steady_clock::now();
        auto result = m_inner->find_by_code(code);
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        
        m_find_histogram.record(static_cast<double>(elapsed));
        
        if (result.is_ok()) {
            m_find_success.inc();
            span->set_ok();
        } else {
            m_find_miss.inc();
            span->attr("found", false);
        }
        
        return result;
    }

    bool exists(const domain::ShortCode& code) override {
        return m_inner->exists(code);
    }

private:
    std::shared_ptr<domain::ILinkRepository> m_inner;
    
    // Histograms
    obs::Histogram& m_save_histogram;
    obs::Histogram& m_find_histogram;
    obs::Histogram& m_remove_histogram;
    
    // Counters
    obs::Counter& m_save_success;
    obs::Counter& m_save_error;
    obs::Counter& m_find_success;
    obs::Counter& m_find_miss;
    obs::Counter& m_remove_success;
    obs::Counter& m_remove_error;
};

} // namespace url_shortener::infrastructure
