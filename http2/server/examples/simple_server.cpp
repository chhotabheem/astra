#include "Http2Server.h"
#include "Http2Request.h"
#include "Http2Response.h"
#include <Router.h>
#include <iostream>
#include <thread>

int main() {
    try {
        http2server::Server server("0.0.0.0", "8080", 2);

        // Middleware: Logging
        server.router().use([](router::IRequest& req, router::IResponse& res, router::Next next) {
            std::cout << "[" << req.method() << "] " << req.path() << std::endl;
            next();
        });

        // Routes
        server.router().get("/", [](router::IRequest& req, router::IResponse& res) {
            res.set_status(200);
            res.set_header("content-type", "text/plain");
            res.write("Hello, HTTP/2 World from Router!");
            res.close();
        });

        server.router().post("/echo", [](router::IRequest& req, router::IResponse& res) {
            res.set_status(200);
            res.write(req.body());
            res.close();
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
