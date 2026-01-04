#pragma once

#include <IResponse.h>
#include <functional>
#include <memory>
#include <optional>
#include <string>

// Forward declarations
namespace astra::observability {
class Span;
}

namespace uri_shortener::service {

/// Error codes for infrastructure failures
enum class InfraError { NONE = 0, CONNECTION_FAILED, TIMEOUT, PROTOCOL_ERROR };

/// Protocol-agnostic operation types
enum class DataServiceOperation { SAVE, FIND, DELETE, EXISTS };

/// Protocol-agnostic request to data service
struct DataServiceRequest {
  DataServiceOperation op;
  std::string entity_id;                              // e.g., short code
  std::string payload;                                // JSON payload for SAVE
  std::shared_ptr<astra::router::IResponse> response; // Response interface
  std::shared_ptr<astra::observability::Span> span;
};

/// Protocol-agnostic response from data service
struct DataServiceResponse {
  bool success = false;

  // Error information (mutually exclusive in practice)
  std::optional<int> domain_error_code;  // Domain-level error
  std::optional<InfraError> infra_error; // Infrastructure error
  std::string error_message;

  // Success payload
  std::string payload; // JSON response
  int http_status = 0; // Original HTTP status (for debugging)

  // Passthrough from request
  std::shared_ptr<astra::router::IResponse> response; // Response interface
  std::shared_ptr<astra::observability::Span> span;
};

/// Callback type for async responses
using DataServiceCallback = std::function<void(DataServiceResponse)>;

} // namespace uri_shortener::service
