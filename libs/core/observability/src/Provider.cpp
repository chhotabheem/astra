#include "ProviderImpl.h"
#include "TracerImpl.h"

#include <Provider.h>
#include <Tracer.h>

namespace astra::observability {

Provider &Provider::instance() {
  static Provider provider;
  return provider;
}

Provider::Provider() : m_impl(std::make_unique<ProviderImpl>()) {
}

Provider::~Provider() = default;

bool Provider::init(const ::observability::Config &config) {
  return m_impl->init(config);
}

bool Provider::shutdown() {
  return m_impl->shutdown();
}

std::shared_ptr<Tracer> Provider::get_tracer(const std::string &name) {
  return std::make_shared<TracerImpl>(name, *m_impl);
}

Provider::Impl &Provider::impl() {
  return *m_impl;
}

bool init(const ::observability::Config &config) {
  return Provider::instance().init(config);
}

bool shutdown() {
  return Provider::instance().shutdown();
}

} // namespace astra::observability
