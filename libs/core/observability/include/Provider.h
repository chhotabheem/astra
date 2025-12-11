#pragma once
#include "Config.h"
#include <memory>

namespace obs {

class ProviderImpl;

class Provider {
public:
    static Provider& instance();
    
    // Initialize with parameters. Returns bool to indicate success/failure.
    bool init(const InitParams& params);
    bool shutdown();
    
    // Public alias for accessing implementation (needed by Metrics.cpp)
    using Impl = ProviderImpl;
    Impl& impl();
    
private:
    Provider();
    ~Provider();
    Provider(const Provider&) = delete;
    Provider& operator=(const Provider&) = delete;
    
    std::unique_ptr<ProviderImpl> m_impl;
};

// Convenience functions
bool init(const InitParams& params);
bool shutdown();

} // namespace obs

