#pragma once

#include <Context.h>
#include <Tracer.h>
#include <memory>
#include <string>

namespace astra::observability {

class ProviderImpl;

/**
 * TracerImpl - OpenTelemetry-backed Tracer implementation
 */
class TracerImpl : public Tracer {
public:
  TracerImpl(std::string name, ProviderImpl &provider);
  ~TracerImpl() override = default;

  // Non-copyable, non-move-assignable (reference member can't be rebound)
  TracerImpl(const TracerImpl &) = delete;
  TracerImpl &operator=(const TracerImpl &) = delete;
  TracerImpl(TracerImpl &&) noexcept = default;
  TracerImpl &operator=(TracerImpl &&) = delete;

  std::shared_ptr<Span> start_span(const std::string &name) override;
  std::shared_ptr<Span> start_span(const std::string &name,
                                   const Context &parent) override;

  const std::string &name() const override {
    return m_name;
  }

private:
  std::string m_name;
  ProviderImpl &m_provider;
};

} // namespace astra::observability
