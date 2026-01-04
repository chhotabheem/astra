#pragma once

#include <Context.h>
#include <any>
#include <cstdint>

namespace astra::execution {

struct Message {
  uint64_t affinity_key;
  astra::observability::Context trace_ctx;
  std::any payload;
};

} // namespace astra::execution
