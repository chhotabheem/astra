#include "UriShortenerMessageHandler.h"
#include <Message.h>
#include <Log.h>
#include <any>
#include "ShortenLink.h"
#include "ResolveLink.h"
#include "DeleteLink.h"

namespace url_shortener {

UriShortenerMessageHandler::UriShortenerMessageHandler(
    std::shared_ptr<application::ShortenLink> shorten,
    std::shared_ptr<application::ResolveLink> resolve,
    std::shared_ptr<application::DeleteLink> del
)
    : m_shorten(std::move(shorten))
    , m_resolve(std::move(resolve))
    , m_delete(std::move(del)) {
}

void UriShortenerMessageHandler::handle(astra::execution::Message& msg) {
    try {
        auto& payload = std::any_cast<UriPayload&>(msg.payload);
        
        std::visit(overloaded{
            [this](HttpRequestMsg& req) {
                processHttpRequest(req);
            },
            [this](DbQueryMsg& query) {
                // Legacy path - should not be used with new architecture
                obs::warn("Received standalone DbQueryMsg - unexpected");
            },
            [this](DbResponseMsg& resp) {
                // Legacy path - should not be used with new architecture
                obs::warn("Received standalone DbResponseMsg - unexpected");
            }
        }, payload);
    } catch (const std::bad_any_cast& e) {
        obs::error("UriShortenerMessageHandler: Invalid payload type");
    }
}

void UriShortenerMessageHandler::processHttpRequest(HttpRequestMsg& req) {
    // Parse request
    auto method = std::string(req.request.method());
    auto path = std::string(req.request.path());
    auto body = std::string(req.request.body());
    
    std::string operation = determine_operation(method, path);
    
    if (operation.empty()) {
        req.response.set_status(404);
        req.response.set_header("Content-Type", "application/json");
        req.response.write(R"({"error": "Not Found"})");
        req.response.close();
        return;
    }
    
    // Extract data
    std::string data;
    if (operation == "shorten") {
        // Parse JSON body for URL
        auto url_start = body.find("\"url\"");
        if (url_start != std::string::npos) {
            auto value_start = body.find(':', url_start);
            auto quote_start = body.find('"', value_start);
            auto quote_end = body.find('"', quote_start + 1);
            if (quote_start != std::string::npos && quote_end != std::string::npos) {
                data = body.substr(quote_start + 1, quote_end - quote_start - 1);
            }
        }
        if (data.empty()) {
            req.response.set_status(400);
            req.response.set_header("Content-Type", "application/json");
            req.response.write(R"({"error": "Missing 'url' field"})");
            req.response.close();
            return;
        }
    } else {
        // Extract short code from path (e.g., /abc123 -> abc123)
        if (path.size() > 1) {
            data = path.substr(1);
        }
    }
    
    // Execute operation (synchronous)
    if (operation == "shorten") {
        application::ShortenLink::Input input{data};
        auto res = m_shorten->execute(input);
        req.response.set_header("Content-Type", "application/json");
        if (res.is_ok()) {
            req.response.set_status(201);
            req.response.write("{\"short_code\": \"" + res.value().short_code + 
                              "\", \"original_url\": \"" + res.value().original_url + "\"}");
        } else {
            req.response.set_status(400);
            req.response.write(R"({"error": "Failed to shorten URL"})");
        }
    } else if (operation == "resolve") {
        application::ResolveLink::Input input{data};
        auto res = m_resolve->execute(input);
        req.response.set_header("Content-Type", "application/json");
        if (res.is_ok()) {
            req.response.set_status(200);
            req.response.write("{\"original_url\": \"" + res.value().original_url + "\"}");
        } else {
            req.response.set_status(404);
            req.response.write(R"({"error": "Short code not found"})");
        }
    } else if (operation == "delete") {
        application::DeleteLink::Input input{data};
        auto res = m_delete->execute(input);
        req.response.set_header("Content-Type", "application/json");
        if (res.is_ok()) {
            req.response.set_status(204);
        } else {
            req.response.set_status(404);
            req.response.write(R"({"error": "Failed to delete"})");
        }
    }
    
    req.response.close();
}

std::string UriShortenerMessageHandler::determine_operation(const std::string& method, const std::string& path) {
    if (method == "POST" && path == "/shorten") {
        return "shorten";
    } else if (method == "GET" && path.size() > 1 && path[0] == '/') {
        return "resolve";
    } else if (method == "DELETE" && path.size() > 1 && path[0] == '/') {
        return "delete";
    }
    return "";
}

} // namespace url_shortener
