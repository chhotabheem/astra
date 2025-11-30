#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Http1Server.h"
#include <thread>
#include <chrono>
#include <boost/asio.hpp>

using namespace std::chrono_literals;
using namespace testing;

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

class Http1ServerTest : public Test {
protected:
    void SetUp() override {
        server_ = std::make_unique<http1::Server>("127.0.0.1", port_, 4);
        server_->handle([](const router::IRequest& req, router::IResponse& res) {
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

        server_thread_ = std::thread([this]{
            server_->run();
        });
        
        // Wait for server to start
        std::this_thread::sleep_for(100ms);
    }

    void TearDown() override {
        server_->stop();
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

    int port_ = 8083; // Use a different port to avoid conflicts
    std::unique_ptr<http1::Server> server_;
    std::thread server_thread_;
};

TEST_F(Http1ServerTest, BasicRequests) {
    auto res = send_request(port_, "GET", "/test");
    EXPECT_THAT(res, HasSubstr("200 OK"));
    
    res = send_request(port_, "GET", "/unknown");
    EXPECT_THAT(res, HasSubstr("404 Not Found"));
}

TEST_F(Http1ServerTest, LargeBody) {
    std::string large_body(1024 * 1024, 'a'); // 1MB body
    auto res = send_request(port_, "POST", "/echo", large_body);
    EXPECT_THAT(res, HasSubstr("200 OK"));
}

TEST_F(Http1ServerTest, ConcurrentRequests) {
    int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &success_count] {
            auto res = send_request(port_, "GET", "/test");
            if (res.find("200 OK") != std::string::npos) {
                success_count++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count, num_threads);
}

TEST_F(Http1ServerTest, StressMemory) {
    int num_requests = 1000;
    for (int i = 0; i < num_requests; ++i) {
        auto res = send_request(port_, "GET", "/test");
        EXPECT_THAT(res, HasSubstr("200 OK"));
    }
}

TEST_F(Http1ServerTest, HighConcurrency) {
    int num_threads = 50; // Higher concurrency
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &success_count] {
            // Each thread sends multiple requests
            for(int j=0; j<10; ++j) {
                auto res = send_request(port_, "GET", "/test");
                if (res.find("200 OK") != std::string::npos) {
                    success_count++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count, num_threads * 10);
}
