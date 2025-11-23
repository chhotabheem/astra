#include <iostream>
#include <Logger.h>
#include <HttpServerFactory.h>
#include <MongoClient.h>

using namespace httpserver;

void rootHandler(const IRequest& req, IResponse& res) {
    LOG_INFO("Handling request for " + req.path());
    res.setStatus(200);
    res.json(R"({"status": "ok", "message": "Welcome to Astra Server"})");
}

void userHandler(const IRequest& req, IResponse& res) {
    auto userId = req.param("id");
    LOG_INFO("Fetching user " + (userId ? *userId : "unknown"));
    
    // Simulate DB lookup
    if (userId == "123") {
        res.setStatus(200);
        res.json(R"({"id": "123", "name": "John Doe", "email": "john@example.com"})");
    } else {
        res.sendError(404, "User not found");
    }
}

int main() {
    // Initialize logger
    logger::Logger::initialize();
    logger::Logger::set_level(logger::Level::INFO);
    
    LOG_INFO("Astra Server starting...");

    try {
        // Create server instance (will use Mock for now, Proxygen later)
        auto server = HttpServerFactory::create(HttpServerFactory::Type::PROXYGEN);
        
        // Register routes
        server->router().GET("/", rootHandler);
        server->router().GET("/users/:id", userHandler);
        
        // Start server
        LOG_INFO("Server listening on 0.0.0.0:8080");
        server->start("0.0.0.0", 8080);
        
    } catch (const std::exception& e) {
        LOG_FATAL("Server failed to start: " + std::string(e.what()));
        return 1;
    }

    logger::Logger::shutdown();
    return 0;
}
