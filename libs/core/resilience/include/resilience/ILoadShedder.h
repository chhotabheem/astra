#pragma once

#include <cstddef>
#include <optional>
#include "LoadShedderGuard.h"

namespace astra::resilience {

class LoadShedderPolicy;

class ILoadShedder {
public:
    virtual ~ILoadShedder() = default;
    
    [[nodiscard]] virtual std::optional<LoadShedderGuard> try_acquire() = 0;
    virtual void update_policy(const LoadShedderPolicy& policy) = 0;
    [[nodiscard]] virtual size_t current_count() const = 0;
    [[nodiscard]] virtual size_t max_concurrent() const = 0;
};

} // namespace astra::resilience
