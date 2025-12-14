#pragma once

#include <string>
#include <variant>
#include <memory>
#include <IRequest.h>
#include <IResponse.h>
#include "DataServiceMessages.h"

namespace url_shortener {

/**
 * @brief HTTP Request Message
 * 
 * Contains protocol-agnostic request and response interfaces.
 */
struct HttpRequestMsg {
    std::shared_ptr<router::IRequest> request;
    std::shared_ptr<router::IResponse> response;
};

/**
 * @brief Database Query Message (Legacy - kept for compatibility)
 */
struct DbQueryMsg {
    std::string operation;
    std::string data;
    std::shared_ptr<router::IResponse> response;
};

/**
 * @brief Database Response Message (Legacy - kept for compatibility)
 */
struct DbResponseMsg {
    std::string result;
    bool success;
    std::string error;
    std::shared_ptr<router::IResponse> response;
};

/**
 * @brief URI Shortener Payload Variant
 * 
 * Type-safe variant for all message types.
 * Used with std::visit for dispatch.
 */
using UriPayload = std::variant<
    HttpRequestMsg, 
    DbQueryMsg, 
    DbResponseMsg,
    service::DataServiceResponse  // New: async response from data service
>;

/**
 * @brief Helper for std::visit
 */
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

} // namespace url_shortener

