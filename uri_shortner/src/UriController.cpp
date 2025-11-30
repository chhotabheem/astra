#include "UriController.h"
#include <iostream>
#include <sstream>

namespace uri_shortener {

UriController::UriController(std::shared_ptr<IUriService> service)
    : service_(std::move(service)) {
}

void UriController::shorten(router::IRequest& req, router::IResponse& res) {
    // 1. Parse Body (Simple text/plain or JSON)
    // For simplicity, assume body IS the long URL if content-type is text/plain
    // Or extract from JSON if application/json
    
    std::string long_url(req.body());
    if (long_url.empty()) {
        res.set_status(400);
        res.write("Missing URL in body");
        res.close();
        return;
    }

    try {
        // 2. Call Service
        std::string short_code = service_->shorten(long_url);

        // 3. Format Response
        res.set_status(201);
        res.set_header("Content-Type", "application/json");
        res.write("{\"short_code\": \"" + short_code + "\"}");
        res.close();
    } catch (const std::exception& e) {
        res.set_status(500);
        res.write(std::string("Internal Error: ") + e.what());
        res.close();
    }
}

void UriController::redirect(router::IRequest& req, router::IResponse& res) {
    // 1. Extract short_code from path params
    auto short_code = req.path_param("code");
    if (short_code.empty()) {
        res.set_status(400);
        res.write("Missing code parameter");
        res.close();
        return;
    }

    // 2. Call Service
    auto long_url = service_->expand(std::string(short_code));

    // 3. Format Response
    if (long_url) {
        res.set_status(302);
        res.set_header("Location", *long_url);
        res.close();
    } else {
        res.set_status(404);
        res.write("URL not found");
        res.close();
    }
}

} // namespace uri_shortener
