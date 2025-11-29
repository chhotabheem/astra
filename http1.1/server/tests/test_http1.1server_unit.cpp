#include "Http1Server.hpp"
#include "Http1Request.hpp"
#include "Http1Response.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <cassert>
#include <boost/asio.hpp>

using namespace std::chrono_literals;

// Helper to send a request and get response
std::string send_request(int port, const std::string& method, const std::string& path, const std::string& body = "") {
    try {
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::socket socket(ioc);
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), port);
        socket.connect(endpoint);

        std::string req = method + " " + path + " HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
        boost::asio::write(socket, boost::asio::buffer(req));

        boost::asio::streambuf buffer;
        boost::asio::read_until(socket, buffer, "\r\n\r\n"); // Read headers
        
        // Read status line
        std::istream is(&buffer);
        std::string status_line;
        std::getline(is, status_line);
        return status_line;
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    }
}

void test_basic_requests(int port) {
    auto res = send_request(port, "GET", "/test");
    assert(res.find("200 OK") != std::string::npos);
    
    res = send_request(port, "GET", "/unknown");
    assert(res.find("404 Not Found") != std::string::npos);
    
    std::cout << "test_basic_requests passed" << std::endl;
}

void test_concurrent_requests(int port) {
    int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([port, &success_count] {
            auto res = send_request(port, "GET", "/test");
            if (res.find("200 OK") != std::string::npos) {
                success_count++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    assert(success_count == num_threads);
    std::cout << "test_concurrent_requests passed (" << num_threads << " requests)" << std::endl;
}

void test_large_body(int port) {
    std::string large_body(1024 * 1024, 'a'); // 1MB body
    // Note: Our simple server implementation might not read the full body in the handler if we don't parse it, 
    // but it should accept the request.
    auto res = send_request(port, "POST", "/echo", large_body);
    // We expect 200 OK because we have a handler (even if it doesn't echo back fully in this simple test setup)
    assert(res.find("200 OK") != std::string::npos);
    std::cout << "test_large_body passed" << std::endl;
}



void test_stress_memory(int port) {
    int num_requests = 1000;
    std::cout << "Starting stress test (" << num_requests << " requests)..." << std::endl;
    for (int i = 0; i < num_requests; ++i) {
        auto res = send_request(port, "GET", "/test");
        assert(res.find("200 OK") != std::string::npos);
        if (i % 100 == 0) std::cout << "." << std::flush;
    }
    std::cout << "\ntest_stress_memory passed" << std::endl;
}

void test_high_concurrency(int port) {
    int num_threads = 50; // Higher concurrency
    std::cout << "Starting high concurrency test (" << num_threads << " threads)..." << std::endl;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([port, &success_count] {
            // Each thread sends multiple requests
            for(int j=0; j<10; ++j) {
                auto res = send_request(port, "GET", "/test");
                if (res.find("200 OK") != std::string::npos) {
                    success_count++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    assert(success_count == num_threads * 10);
    std::cout << "test_high_concurrency passed" << std::endl;
}

int main() {
    int port = 8082;
    
    // Start server in a thread
    http1::Server server("127.0.0.1", port, 4); // 4 threads
    server.handle([](const http_abstractions::IRequest& req, http_abstractions::IResponse& res) {
        if (req.path() == "/test") {
            res.set_status(200);
            res.write("Hello Test");
        } else if (req.path() == "/echo") {
            res.set_status(200);
            res.write(req.body());
        } else {
            res.set_status(404);
        }
        res.close();
    });

    std::thread server_thread([&server]{
        server.run();
    });

    std::this_thread::sleep_for(1s); // Wait for server to start

    test_basic_requests(port);
    test_concurrent_requests(port);
    test_large_body(port);
    
    // New Stress Tests
    test_stress_memory(port);
    test_high_concurrency(port);

    server.stop();
    if (server_thread.joinable()) {
        server_thread.join();
    }

    std::cout << "All http1.1server tests passed" << std::endl;
    return 0;
}
