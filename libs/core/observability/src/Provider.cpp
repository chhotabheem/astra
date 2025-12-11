#include <Provider.h>
#include "ProviderImpl.h"

namespace obs {

Provider& Provider::instance() {
    static Provider provider;
    return provider;
}

Provider::Provider() : m_impl(std::make_unique<ProviderImpl>()) {}

Provider::~Provider() = default;

bool Provider::init(const InitParams& params) {
    return m_impl->init(params);
}

bool Provider::shutdown() {
    return m_impl->shutdown();
}

Provider::Impl& Provider::impl() {
    return *m_impl;
}

bool init(const InitParams& params) {
    return Provider::instance().init(params);
}

bool shutdown() {
    return Provider::instance().shutdown();
}

} // namespace obs
