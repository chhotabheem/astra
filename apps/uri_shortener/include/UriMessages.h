#pragma once

#include <string>
#include <variant>
#include <memory>
#include "Http2Request.h"
#include "Http2Response.h"

namespace url_shortener {

/**
 * @brief HTTP Request Message
 * 
 * Submitted by RequestHandler when an HTTP request arrives.
 * Processed by MessageHandler::onHttpRequest.
 */
struct HttpRequestMsg {
    http2server::Request request;
    http2server::Response response;
};

/**
 * @brief Database Query Message
 * 
 * Created by MessageHandler after parsing HTTP request.
 * Represents: shorten, resolve, or delete operation.
 */
struct DbQueryMsg {
    std::string operation;  // "shorten", "resolve", "delete"
    std::string data;       // URL to shorten or short code to lookup
    http2server::Response response;
};

/**
 * @brief Database Response Message
 * 
 * Created after database operation completes.
 * Contains result to send back as HTTP response.
 */
struct DbResponseMsg {
    std::string result;
    bool success;
    std::string error;
    http2server::Response response;
};

/**
 * @brief URI Shortener Payload Variant
 * 
 * Type-safe variant for all message types.
 * Used with std::visit for dispatch.
 */
using UriPayload = std::variant<HttpRequestMsg, DbQueryMsg, DbResponseMsg>;

/**
 * @brief Helper for std::visit
 */
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

} // namespace url_shortener
