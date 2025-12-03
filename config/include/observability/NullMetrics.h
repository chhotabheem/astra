#pragma once
#include "observability/IConfigMetrics.h"

namespace config {

class NullMetrics : public IConfigMetrics {
public:
    void incrementReloadSuccess() override {}
    void incrementReloadFailure() override {}
};

}
