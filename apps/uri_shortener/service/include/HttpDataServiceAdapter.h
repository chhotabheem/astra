#pragma once

#include "IDataServiceAdapter.h"
#include "Http2ClientPool.h"
#include <string>

namespace url_shortener::service {

/// HTTP/2 implementation of the data service adapter
/// Translates protocol-agnostic requests to HTTP/2 calls  
class HttpDataServiceAdapter : public IDataServiceAdapter {
public:
    /// Configuration for the adapter
    struct Config {
        std::string base_path = "/api/v1/links";  // Base API path
    };
    
    /// Construct with a client pool reference
    /// @param client_pool Pool of HTTP/2 clients
    explicit HttpDataServiceAdapter(http2client::Http2ClientPool& client_pool);
    
    /// Construct with a client pool and custom configuration
    HttpDataServiceAdapter(http2client::Http2ClientPool& client_pool, Config config);
    
    ~HttpDataServiceAdapter() override = default;
    
    /// Execute request by translating to HTTP
    void execute(DataServiceRequest request, DataServiceCallback callback) override;

private:
    /// Translate operation to HTTP method
    static std::string operation_to_method(DataServiceOperation op);
    
    /// Build HTTP path for the operation
    std::string build_path(DataServiceOperation op, const std::string& entity_id) const;
    
    /// Map HTTP status code to domain error code (0 = success)
    static int map_http_status_to_error(int status_code);
    
    http2client::Http2ClientPool& m_client_pool;
    Config m_config;
};

} // namespace url_shortener::service
