#include "HttpDataServiceAdapter.h"
#include "Http2Client.h"
#include <Log.h>

namespace url_shortener::service {

HttpDataServiceAdapter::HttpDataServiceAdapter(astra::http2::Http2ClientPool& client_pool)
    : m_client_pool(client_pool), m_config{} {}

HttpDataServiceAdapter::HttpDataServiceAdapter(
    astra::http2::Http2ClientPool& client_pool,
    Config config
) : m_client_pool(client_pool), m_config(std::move(config)) {}

void HttpDataServiceAdapter::execute(DataServiceRequest request, DataServiceCallback callback) {
    // Build HTTP request parameters
    std::string method = operation_to_method(request.op);
    std::string path = build_path(request.op, request.entity_id);
    
    // Prepare headers
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    
    // Add trace context if span is available
    if (request.span) {
        // TODO: Add traceparent header from span context
        // headers["traceparent"] = request.span->context().to_traceparent();
    }
    
    // Capture request fields for callback
    auto response = request.response;
    auto span = request.span;
    
    // Get a client from pool and submit
    m_client_pool.get().submit(
        method,
        path,
        request.payload,
        headers,
        [callback, response, span](const astra::http2::ClientResponse& resp, const astra::http2::Error& err) {
            DataServiceResponse ds_resp;
            ds_resp.response = response;
            ds_resp.span = span;
            
            // Check for infrastructure error
            if (err) {
                ds_resp.success = false;
                ds_resp.error_message = err.message;
                
                // Map error codes to InfraError
                switch (err.code) {
                    case 1:  // Not connected
                        ds_resp.infra_error = InfraError::CONNECTION_FAILED;
                        break;
                    case 2:  // Timeout
                        ds_resp.infra_error = InfraError::TIMEOUT;
                        break;
                    default:
                        ds_resp.infra_error = InfraError::PROTOCOL_ERROR;
                        break;
                }
                
                callback(std::move(ds_resp));
                return;
            }
            
            // Process HTTP response
            ds_resp.http_status = resp.status_code();
            ds_resp.payload = resp.body();
            
            // Map HTTP status to success/error
            if (resp.status_code() >= 200 && resp.status_code() < 300) {
                ds_resp.success = true;
            } else {
                ds_resp.success = false;
                ds_resp.domain_error_code = map_http_status_to_error(resp.status_code());
                ds_resp.error_message = resp.body();
            }
            
            callback(std::move(ds_resp));
        }
    );
}

std::string HttpDataServiceAdapter::operation_to_method(DataServiceOperation op) {
    switch (op) {
        case DataServiceOperation::SAVE:   return "POST";
        case DataServiceOperation::FIND:   return "GET";
        case DataServiceOperation::DELETE: return "DELETE";
        case DataServiceOperation::EXISTS: return "HEAD";
        default:                           return "GET";
    }
}

std::string HttpDataServiceAdapter::build_path(DataServiceOperation op, const std::string& entity_id) const {
    switch (op) {
        case DataServiceOperation::SAVE:
            return m_config.base_path;
        case DataServiceOperation::FIND:
        case DataServiceOperation::DELETE:
        case DataServiceOperation::EXISTS:
            return m_config.base_path + "/" + entity_id;
        default:
            return m_config.base_path;
    }
}

int HttpDataServiceAdapter::map_http_status_to_error(int status_code) {
    // Map HTTP status codes to domain error codes
    // Using simple integer codes that can be cast to DomainError later
    switch (status_code) {
        case 404: return 1;  // NOT_FOUND
        case 409: return 2;  // ALREADY_EXISTS (conflict)
        case 400: return 3;  // INVALID_REQUEST
        case 500: return 4;  // INTERNAL_ERROR
        case 503: return 5;  // SERVICE_UNAVAILABLE
        default:  return 99; // UNKNOWN_ERROR
    }
}

} // namespace url_shortener::service
