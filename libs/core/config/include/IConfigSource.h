#pragma once
#include <string>
#include <functional>

namespace config {

class IConfigSource {
public:
    using ChangeCallback = std::function<void(const std::string&)>;
    
    virtual ~IConfigSource() = default;
    
    virtual std::string fetchConfig() = 0;
    virtual void watchForChanges(ChangeCallback callback) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};

}
