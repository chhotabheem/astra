#pragma once

#include <IMessageHandler.h>
#include "UriMessages.h"
#include "application/use_cases/ShortenLink.h"
#include "application/use_cases/ResolveLink.h"
#include "application/use_cases/DeleteLink.h"
#include <obs/Context.h>
#include <memory>

namespace url_shortener {

/**
 * @brief Message Handler for URI Shortener
 * 
 * Implements IMessageHandler. Uses std::visit to dispatch
 * based on UriPayload variant type. Processes synchronously.
 */
class UriShortenerMessageHandler : public astra::execution::IMessageHandler {
public:
    UriShortenerMessageHandler(
        std::shared_ptr<application::ShortenLink> shorten,
        std::shared_ptr<application::ResolveLink> resolve,
        std::shared_ptr<application::DeleteLink> del
    );
    
    /**
     * @brief Handle a message from the pool
     * 
     * Casts payload to UriPayload and dispatches via std::visit.
     */
    void handle(astra::execution::Message& msg) override;

private:
    /**
     * @brief Process HTTP request synchronously
     * 
     * Parses request, executes operation, sends response.
     */
    void processHttpRequest(HttpRequestMsg& req);
    
    std::string determine_operation(const std::string& method, const std::string& path);
    
    std::shared_ptr<application::ShortenLink> m_shorten;
    std::shared_ptr<application::ResolveLink> m_resolve;
    std::shared_ptr<application::DeleteLink> m_delete;
};

} // namespace url_shortener
