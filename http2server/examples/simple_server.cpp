#include "http2server/server.hpp"
#include <http2server/request.hpp>
#include <http2server/response.hpp>
#include <iostream>
#include <thread>

int main() {
    try {
        http2server::Server server("0.0.0.0", "8080", 2);

        server.handle("GET", "/", [](const http2server::Request& req, http2server::Response& res) {
            res.set_status(200);
            res.set_header("content-type", "text/plain");
            res.write("Hello, HTTP/2 World!");
            res.close();
        });

        server.handle("POST", "/echo", [](const http2server::Request& req, http2server::Response& res) {
            res.set_status(200);
            res.write(req.body());
            res.close();
        });

        std::cout << "Starting server on 0.0.0.0:8080..." << std::endl;
        
        // Run in a separate thread to allow stopping (example)
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
