/// @file ObservableLinkRepository.h
/// @brief Decorator that adds observability to any ILinkRepository

#pragma once

#include "ILinkRepository.h"

#include <Log.h>
#include <MetricsRegistry.h>
#include <Provider.h>
#include <Span.h>
#include <Tracer.h>
#include <chrono>
#include <memory>

namespace uri_shortener::infrastructure {

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
  explicit ObservableLinkRepository(
      std::shared_ptr<domain::ILinkRepository> inner)
      : m_inner(std::move(inner)),
        m_tracer(obs::Provider::instance().get_tracer("uri-shortener")) {
    // Register all metrics using MetricsRegistry pattern
    m_metrics.duration_histogram("save", "link_repo.save.duration")
        .duration_histogram("find", "link_repo.find.duration")
        .duration_histogram("remove", "link_repo.remove.duration")
        .counter("save_success", "link_repo.save.success")
        .counter("save_error", "link_repo.save.error")
        .counter("find_success", "link_repo.find.success")
        .counter("find_miss", "link_repo.find.miss")
        .counter("remove_success", "link_repo.remove.success")
        .counter("remove_error", "link_repo.remove.error");
  }

  astra::outcome::Result<void, domain::DomainError>
  save(const domain::ShortLink &link) override {
    auto span = m_tracer->start_span("LinkRepository.save");
    span->attr("short_code", std::string(link.code().value()));

    auto start = std::chrono::steady_clock::now();
    auto result = m_inner->save(link);
    auto duration = std::chrono::steady_clock::now() - start;

    m_metrics.duration_histogram("save").record(duration);

    if (result.is_ok()) {
      m_metrics.counter("save_success").inc();
      span->set_status(obs::StatusCode::Ok);
      obs::debug("Link saved", {{"code", link.code().value()}});
    } else {
      m_metrics.counter("save_error").inc();
      span->set_status(obs::StatusCode::Error, "save failed");
      obs::warn("Save failed", {{"code", link.code().value()}});
    }

    span->end();
    return result;
  }

  astra::outcome::Result<void, domain::DomainError>
  remove(const domain::ShortCode &code) override {
    auto span = m_tracer->start_span("LinkRepository.remove");
    span->attr("short_code", std::string(code.value()));

    auto start = std::chrono::steady_clock::now();
    auto result = m_inner->remove(code);
    auto duration = std::chrono::steady_clock::now() - start;

    m_metrics.duration_histogram("remove").record(duration);

    if (result.is_ok()) {
      m_metrics.counter("remove_success").inc();
      span->set_status(obs::StatusCode::Ok);
    } else {
      m_metrics.counter("remove_error").inc();
      span->set_status(obs::StatusCode::Error, "remove failed");
    }

    span->end();
    return result;
  }

  astra::outcome::Result<domain::ShortLink, domain::DomainError>
  find_by_code(const domain::ShortCode &code) override {
    auto span = m_tracer->start_span("LinkRepository.find_by_code");
    span->attr("short_code", std::string(code.value()));

    auto start = std::chrono::steady_clock::now();
    auto result = m_inner->find_by_code(code);
    auto duration = std::chrono::steady_clock::now() - start;

    m_metrics.duration_histogram("find").record(duration);

    if (result.is_ok()) {
      m_metrics.counter("find_success").inc();
      span->set_status(obs::StatusCode::Ok);
    } else {
      m_metrics.counter("find_miss").inc();
      span->attr("found", "false");
    }

    span->end();
    return result;
  }

  bool exists(const domain::ShortCode &code) override {
    return m_inner->exists(code);
  }

private:
  std::shared_ptr<domain::ILinkRepository> m_inner;
  std::shared_ptr<obs::Tracer> m_tracer;
  obs::MetricsRegistry m_metrics; // ONE member variable for ALL metrics
};

} // namespace uri_shortener::infrastructure
