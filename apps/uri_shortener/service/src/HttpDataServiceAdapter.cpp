#include "HttpDataServiceAdapter.h"

#include <Log.h>

namespace uri_shortener::service {

HttpDataServiceAdapter::HttpDataServiceAdapter(
    astra::http2::Http2Client &http2_client,
    astra::service_discovery::IServiceResolver &resolver,
    std::string service_name, Config config)
    : m_http2_client(http2_client), m_resolver(resolver),
      m_service_name(std::move(service_name)), m_config(std::move(config)) {
}

HttpDataServiceAdapter::HttpDataServiceAdapter(
    astra::http2::Http2Client &http2_client,
    astra::service_discovery::IServiceResolver &resolver,
    std::string service_name)
    : HttpDataServiceAdapter(http2_client, resolver, std::move(service_name),
                             Config{}) {
}

void HttpDataServiceAdapter::execute(DataServiceRequest request,
                                     DataServiceCallback callback) {
  std::string method = operation_to_method(request.op);
  std::string path = build_path(request.op, request.entity_id);

  std::map<std::string, std::string> headers;
  headers["Content-Type"] = "application/json";

  if (request.span) {
    // Span context for tracing
  }

  auto response = request.response;
  auto span = request.span;

  auto [host, port] = m_resolver.resolve(m_service_name);

  m_http2_client.submit(
      host, port, method, path, request.payload, headers,
      [callback, response,
       span](astra::outcome::Result<astra::http2::Http2ClientResponse,
                                    astra::http2::Http2ClientError>
                 result) {
        DataServiceResponse ds_resp;
        ds_resp.response = response;
        ds_resp.span = span;

        if (result.is_err()) {
          ds_resp.success = false;
          auto err = result.error();

          switch (err) {
          case astra::http2::Http2ClientError::ConnectionFailed:
          case astra::http2::Http2ClientError::NotConnected:
            ds_resp.infra_error = InfraError::CONNECTION_FAILED;
            ds_resp.error_message = "Connection failed";
            break;
          case astra::http2::Http2ClientError::RequestTimeout:
            ds_resp.infra_error = InfraError::TIMEOUT;
            ds_resp.error_message = "Request timeout";
            break;
          case astra::http2::Http2ClientError::StreamClosed:
          case astra::http2::Http2ClientError::SubmitFailed:
            ds_resp.infra_error = InfraError::PROTOCOL_ERROR;
            ds_resp.error_message = "Protocol error";
            break;
          }

          callback(std::move(ds_resp));
          return;
        }

        auto resp = result.value();
        ds_resp.http_status = resp.status_code();
        ds_resp.payload = resp.body();

        if (resp.status_code() >= 200 && resp.status_code() < 300) {
          ds_resp.success = true;
        } else {
          ds_resp.success = false;
          ds_resp.domain_error_code =
              map_http_status_to_error(resp.status_code());
          ds_resp.error_message = resp.body();
        }

        callback(std::move(ds_resp));
      });
}

std::string
HttpDataServiceAdapter::operation_to_method(DataServiceOperation op) {
  switch (op) {
  case DataServiceOperation::SAVE:
    return "POST";
  case DataServiceOperation::FIND:
    return "GET";
  case DataServiceOperation::DELETE:
    return "DELETE";
  case DataServiceOperation::EXISTS:
    return "HEAD";
  default:
    return "GET";
  }
}

std::string
HttpDataServiceAdapter::build_path(DataServiceOperation op,
                                   const std::string &entity_id) const {
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
  switch (status_code) {
  case 404:
    return 1; // NOT_FOUND
  case 409:
    return 2; // ALREADY_EXISTS (conflict)
  case 400:
    return 3; // INVALID_REQUEST
  case 500:
    return 4; // INTERNAL_ERROR
  case 503:
    return 5; // SERVICE_UNAVAILABLE
  default:
    return 99; // UNKNOWN_ERROR
  }
}

} // namespace uri_shortener::service
