#include "Http2Client.hpp"
#include "Http2ClientResponse.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Configure client
    http2client::ClientConfig config;
    config.host = "localhost";
    config.port = "8080"; // Assuming simple_server runs on 8080
    config.connect_timeout = std::chrono::seconds(2);
    config.request_timeout = std::chrono::seconds(5);
    config.max_retries = 3;

    std::cout << "Initializing client..." << std::endl;
    http2client::Client client(config);

    // Wait a bit for connection (optional, but good for demo)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (client.is_connected()) {
        std::cout << "Connected!" << std::endl;
    } else {
        std::cout << "Connecting..." << std::endl;
    }

    // Send a GET request
    std::cout << "Sending GET /ping..." << std::endl;
    client.get("/ping", [](const http2client::Response& res, const http2client::Error& err) {
        if (err) {
            std::cerr << "Request failed: " << err.message << " (code: " << err.code << ")" << std::endl;
        } else {
            std::cout << "Response received!" << std::endl;
            std::cout << "Status: " << res.status_code() << std::endl;
            std::cout << "Body: " << res.body() << std::endl;
        }
    });

    // Send a POST request
    std::cout << "Sending POST /echo..." << std::endl;
    client.post("/echo", "Hello HTTP/2", [](const http2client::Response& res, const http2client::Error& err) {
        if (err) {
            std::cerr << "Request failed: " << err.message << std::endl;
        } else {
            std::cout << "Response received!" << std::endl;
            std::cout << "Status: " << res.status_code() << std::endl;
            std::cout << "Body: " << res.body() << std::endl;
        }
    });

    // Keep main thread alive to receive callbacks
    std::cout << "Waiting for responses..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    return 0;
}
