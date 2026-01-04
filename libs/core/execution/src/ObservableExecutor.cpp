#include "ObservableExecutor.h"

namespace astra::execution {

ObservableExecutor::ObservableExecutor(IExecutor &inner) : m_inner(inner) {
  m_metrics.counter("submitted", "executor.submitted")
      .gauge("queue_depth", "executor.queue_depth");
}

void ObservableExecutor::submit(Message msg) {
  m_metrics.counter("submitted").inc();
  m_metrics.gauge("queue_depth").add(1);
  m_inner.submit(std::move(msg));
}

} // namespace astra::execution
