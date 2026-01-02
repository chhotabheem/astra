#pragma once

#include "DataServiceMessages.h"

namespace uri_shortener::service {

/// Protocol-agnostic interface for data service operations
/// Hides HTTP, gRPC, or any other protocol details
class IDataServiceAdapter {
public:
  virtual ~IDataServiceAdapter() = default;

  /// Execute a data service request asynchronously
  /// @param request The request to execute
  /// @param callback Called when operation completes (success or failure)
  virtual void execute(DataServiceRequest request,
                       DataServiceCallback callback) = 0;
};

} // namespace uri_shortener::service
