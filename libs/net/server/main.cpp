#include <iostream>
#include <Log.h>
#include <HttpServerFactory.h>
#include <MongoClient.h>

using namespace httpserver;

void rootHandler(const IRequest& req, IResponse& res) {
    obs::info("Handling request for " + req.path());
    res.setStatus(200);
    res.json(R"({"status": "ok", "message": "Welcome to Astra Server"})");
}

void userHandler(const IRequest& req, IResponse& res) {
    auto userId = req.param("id");
    obs::info("Fetching user " + (userId ? *userId : "unknown"));
    
    // Simulate DB lookup
    if (userId == "123") {
        res.setStatus(200);
        res.json(R"({"id": "123", "name": "John Doe", "email": "john@example.com"})");
    } else {
        res.sendError(404, "User not found");
    }
}

int main() {
    obs::info("Astra Server starting...");

    try {
        // Create server instance (will use Mock for now, Proxygen later)
        auto server = HttpServerFactory::create(HttpServerFactory::Type::PROXYGEN);
        
        // Register routes
        server->router().GET("/", rootHandler);
        server->router().GET("/users/:id", userHandler);
        
        // Start server
        obs::info("Server listening on 0.0.0.0:8080");
        server->start("0.0.0.0", 8080);
        
    } catch (const std::exception& e) {
        obs::fatal("Server failed to start: " + std::string(e.what()));
        return 1;
    }

    return 0;
}

