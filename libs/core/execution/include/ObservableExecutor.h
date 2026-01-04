#pragma once

#include "IExecutor.h"

#include <MetricsRegistry.h>

namespace astra::execution {

class ObservableExecutor : public IExecutor {
public:
  explicit ObservableExecutor(IExecutor &inner);
  ~ObservableExecutor() override = default;

  void submit(Message msg) override;

private:
  IExecutor &m_inner;
  obs::MetricsRegistry m_metrics;
};

} // namespace astra::execution
