#pragma once

#include "Http2Client.h"
#include "IDataServiceAdapter.h"
#include "IServiceResolver.h"

#include <string>

namespace uri_shortener::service {

/// HTTP/2 implementation of the data service adapter
/// Translates protocol-agnostic requests to HTTP/2 calls
class HttpDataServiceAdapter : public IDataServiceAdapter {
public:
  /// Configuration for the adapter
  struct Config {
    std::string base_path = "/api/v1/links"; // Base API path
  };

  /// Construct with Http2 client, service resolver, service name, and config
  HttpDataServiceAdapter(astra::http2::Http2Client &http2_client,
                         astra::service_discovery::IServiceResolver &resolver,
                         std::string service_name, Config config);

  /// Construct with default config
  HttpDataServiceAdapter(astra::http2::Http2Client &http2_client,
                         astra::service_discovery::IServiceResolver &resolver,
                         std::string service_name);

  ~HttpDataServiceAdapter() override = default;

  /// Execute request by translating to HTTP
  void execute(DataServiceRequest request,
               DataServiceCallback callback) override;

private:
  /// Translate operation to HTTP method
  static std::string operation_to_method(DataServiceOperation op);

  /// Build HTTP path for the operation
  std::string build_path(DataServiceOperation op,
                         const std::string &entity_id) const;

  /// Map HTTP status code to domain error code (0 = success)
  static int map_http_status_to_error(int status_code);

  astra::http2::Http2Client &m_http2_client;
  astra::service_discovery::IServiceResolver &m_resolver;
  std::string m_service_name;
  Config m_config;
};

} // namespace uri_shortener::service
