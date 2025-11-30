#include "Http1Server.h"
#include "Http2Server.h"
#include "RedisClient.h"
#include "MongoClient.h"
#include "RedisUriRepository.h"
#include "MongoUriRepository.h"
#include "UriService.h"
#include "UriController.h"
#include <iostream>
#include <thread>
#include <csignal>

using namespace uri_shortener;

std::atomic<bool> running{true};

void signal_handler(int) {
    running = false;
}

int main() {
    try {
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // 1. Initialize Infrastructure
        std::cout << "Initializing Infrastructure..." << std::endl;
        auto redis = std::make_shared<redisclient::RedisClient>("tcp://127.0.0.1:6379");
        auto mongo = std::make_shared<mongoclient::MongoClient>();
        mongo->connect("mongodb://localhost:27017");

        // 2. Initialize Layers
        std::cout << "Initializing Layers..." << std::endl;
        // Use Redis as primary repository for now (ID Gen + Cache)
        // In a real scenario, we might wrap both in a CompositeRepository
        auto redis_repo = std::make_shared<RedisUriRepository>(redis);
        auto mongo_repo = std::make_shared<MongoUriRepository>(mongo);
        
        // Service uses Redis for ID gen and fast lookup
        // TODO: Add dual-write logic in Service or CompositeRepo
        auto service = std::make_shared<UriService>(redis_repo);
        auto controller = std::make_shared<UriController>(service);

        // 3. Initialize Router
        std::cout << "Configuring Router..." << std::endl;
        // We need a shared router configuration or configure both servers
        // Since Http1Server and Http2Server have their own internal routers (or access to one),
        // we need to register routes on both.
        
        auto setup_routes = [&](router::Router& r) {
            r.post("/shorten", [controller](router::IRequest& req, router::IResponse& res) {
                controller->shorten(req, res);
            });
            
            r.get("/:code", [controller](router::IRequest& req, router::IResponse& res) {
                controller->redirect(req, res);
            });
        };

        // 4. Initialize Servers
        std::cout << "Starting Servers..." << std::endl;
        http1::Server http1_server("0.0.0.0", 8081, 2);
        http2server::Server http2_server("0.0.0.0", "8080", 4);

        setup_routes(http1_server.router());
        setup_routes(http2_server.router());

        // 5. Run
        std::thread t1([&] { http1_server.run(); });
        std::thread t2([&] { http2_server.run(); });

        std::cout << "URI Shortener Service Running:" << std::endl;
        std::cout << "  - HTTP/2 (Traffic): http://localhost:8080" << std::endl;
        std::cout << "  - HTTP/1.1 (Health): http://localhost:8081" << std::endl;

        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cout << "Stopping servers..." << std::endl;
        http1_server.stop();
        http2_server.stop();
        
        if (t1.joinable()) t1.join();
        if (t2.joinable()) t2.join();

        std::cout << "Goodbye!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
