#pragma once

namespace config {

class IConfigMetrics {
public:
    virtual ~IConfigMetrics() = default;
    
    virtual void incrementReloadSuccess() = 0;
    virtual void incrementReloadFailure() = 0;
};

}
