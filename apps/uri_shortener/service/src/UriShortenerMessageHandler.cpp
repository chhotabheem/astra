#include "UriShortenerMessageHandler.h"

#include <IRequest.h>
#include <IResponse.h>
#include <Log.h>
#include <Message.h>
#include <Span.h>
#include <any>
#include <functional>
#include <utility>

namespace uri_shortener {

// Type alias for the payload pair
using RequestResponsePair =
    std::pair<std::shared_ptr<astra::router::IRequest>,
              std::shared_ptr<astra::router::IResponse>>;

UriShortenerMessageHandler::UriShortenerMessageHandler(
    std::shared_ptr<service::IDataServiceAdapter> adapter)
    : m_adapter(std::move(adapter)) {
}

void UriShortenerMessageHandler::setResponseExecutor(
    astra::execution::IExecutor &executor) {
  m_response_executor = &executor;
}

void UriShortenerMessageHandler::handle(astra::execution::Message &msg) {
  try {
    // Try pair payload first (new format - HTTP request)
    auto &pair = std::any_cast<RequestResponsePair &>(msg.payload);
    processHttpRequest(pair.first, pair.second, msg.affinity_key,
                       msg.trace_ctx);
  } catch (const std::bad_any_cast &) {
    try {
      // Try DataServiceResponse (callback from adapter)
      auto &resp = std::any_cast<service::DataServiceResponse &>(msg.payload);
      processDataServiceResponse(resp);
    } catch (const std::bad_any_cast &) {
      obs::error("Unknown message payload type");
    }
  }
}

void UriShortenerMessageHandler::processHttpRequest(
    std::shared_ptr<astra::router::IRequest> req,
    std::shared_ptr<astra::router::IResponse> res, uint64_t affinity_key,
    obs::Context &trace_ctx) {
  std::string method(req->method());
  std::string path(req->path());
  std::string body(req->body());

  std::string operation = determine_operation(method, path);

  if (operation.empty()) {
    res->set_status(404);
    res->set_header("Content-Type", "application/json");
    res->write(R"({"error": "Not found"})");
    res->close();
    return;
  }

  // Extract data based on operation
  std::string entity_id;
  std::string payload;

  if (operation == "shorten") {
    // Parse JSON body for URL
    auto url_start = body.find("\"url\"");
    if (url_start != std::string::npos) {
      auto value_start = body.find(':', url_start);
      auto quote_start = body.find('"', value_start);
      auto quote_end = body.find('"', quote_start + 1);
      if (quote_start != std::string::npos && quote_end != std::string::npos) {
        payload = body.substr(quote_start + 1, quote_end - quote_start - 1);
      }
    }
    if (payload.empty()) {
      res->set_status(400);
      res->set_header("Content-Type", "application/json");
      res->write(R"({"error": "Missing 'url' field"})");
      res->close();
      return;
    }
  } else {
    // Extract short code from path (e.g., /abc123 -> abc123)
    if (path.size() > 1) {
      entity_id = path.substr(1);
    }
  }

  // Create DataServiceRequest
  service::DataServiceRequest ds_req{
      to_data_service_op(operation), entity_id, payload,
      res,    // Pass shared_ptr<IResponse>
      nullptr // No span for now
  };

  // If no adapter configured, respond with error
  if (!m_adapter) {
    obs::warn("No adapter configured - responding with error");
    res->set_status(503);
    res->set_header("Content-Type", "application/json");
    res->write(R"({"error": "Service not configured"})");
    res->close();
    return;
  }

  // Capture affinity_key and trace_ctx for callback
  auto captured_affinity_key = affinity_key;
  auto captured_trace_ctx = trace_ctx;
  auto response_executor = m_response_executor;

  // Call adapter with callback that submits response to executor
  m_adapter->execute(ds_req,
                     [response_executor, captured_affinity_key,
                      captured_trace_ctx](service::DataServiceResponse resp) {
                       // Submit response back to executor for processing
                       astra::execution::Message response_msg;
                       response_msg.affinity_key = captured_affinity_key;
                       response_msg.trace_ctx = captured_trace_ctx;
                       response_msg.payload = std::move(resp);

                       if (response_executor) {
                         response_executor->submit(std::move(response_msg));
                       }
                     });
}

void UriShortenerMessageHandler::processDataServiceResponse(
    service::DataServiceResponse &resp) {
  // Check if client is still connected
  if (!resp.response || !resp.response->is_alive()) {
    obs::warn("Client disconnected before response could be sent");
    return;
  }

  auto &response = *resp.response;

  if (resp.success) {
    response.set_status(resp.http_status > 0 ? resp.http_status : 200);
    response.set_header("Content-Type", "application/json");
    if (!resp.payload.empty()) {
      response.write(resp.payload);
    }
  } else {
    int status = 500;
    if (resp.infra_error.has_value()) {
      switch (resp.infra_error.value()) {
      case service::InfraError::TIMEOUT:
        status = 504;
        break;
      case service::InfraError::CONNECTION_FAILED:
        status = 502;
        break;
      default:
        status = 503;
        break;
      }
    } else if (resp.domain_error_code.has_value()) {
      switch (resp.domain_error_code.value()) {
      case 1:
        status = 404;
        break;
      case 2:
        status = 409;
        break;
      case 3:
        status = 400;
        break;
      default:
        status = 500;
        break;
      }
    }

    response.set_status(status);
    response.set_header("Content-Type", "application/json");
    response.write("{\"error\": \"" + resp.error_message + "\"}");
  }

  response.close();
}

std::string
UriShortenerMessageHandler::determine_operation(const std::string &method,
                                                const std::string &path) {
  if (method == "POST" && path == "/shorten") {
    return "shorten";
  } else if (method == "GET" && path.size() > 1 && path[0] == '/') {
    return "resolve";
  } else if (method == "DELETE" && path.size() > 1 && path[0] == '/') {
    return "delete";
  }
  return "";
}

service::DataServiceOperation
UriShortenerMessageHandler::to_data_service_op(const std::string &operation) {
  if (operation == "shorten") {
    return service::DataServiceOperation::SAVE;
  } else if (operation == "resolve") {
    return service::DataServiceOperation::FIND;
  } else if (operation == "delete") {
    return service::DataServiceOperation::DELETE;
  }
  return service::DataServiceOperation::FIND;
}

} // namespace uri_shortener
