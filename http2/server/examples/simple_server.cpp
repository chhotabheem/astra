#include "Http2Server.hpp"
#include "Http2Request.hpp"
#include "Http2Response.hpp"
#include <router/Router.hpp>
#include <iostream>
#include <thread>

int main() {
    try {
        http2server::Server server("0.0.0.0", "8080", 2);
        router::Router router;

        // Middleware: Logging
        router.use([](const http_abstractions::IRequest& req, http_abstractions::IResponse& res, router::Next next) {
            std::cout << "[" << req.method() << "] " << req.path() << std::endl;
            next();
        });

        // Routes
        router.get("/", [](const http_abstractions::IRequest& req, http_abstractions::IResponse& res) {
            res.set_status(200);
            res.set_header("content-type", "text/plain");
            res.write("Hello, HTTP/2 World from Router!");
            res.close();
        });

        router.post("/echo", [](const http_abstractions::IRequest& req, http_abstractions::IResponse& res) {
            res.set_status(200);
            res.write(req.body());
            res.close();
        });

        // Delegate all requests to Router
        server.handle("*", "/", [&router](const http2server::Request& req, http2server::Response& res) {
            router.dispatch(req, res);
        });

        std::cout << "Starting server on 0.0.0.0:8080..." << std::endl;
        
        std::thread t([&server]() {
            server.run();
        });

        t.join();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
