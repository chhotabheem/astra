#include <Provider.h>
#include "ProviderImpl.h"

namespace obs {

Provider& Provider::instance() {
    static Provider provider;
    return provider;
}

Provider::Provider() : m_impl(std::make_unique<ProviderImpl>()) {}

Provider::~Provider() = default;

bool Provider::init(const Config& config) {
    return m_impl->init(config);
}

bool Provider::shutdown() {
    return m_impl->shutdown();
}

Provider::Impl& Provider::impl() {
    return *m_impl;
}

bool init(const Config& config) {
    return Provider::instance().init(config);
}

bool shutdown() {
    return Provider::instance().shutdown();
}

} // namespace obs
