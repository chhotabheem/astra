#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Http2Server.h"
#include "Http2Request.h"
#include "Http2Response.h"
#include <thread>
#include <chrono>

using namespace testing;
using namespace std::chrono_literals;

namespace {

// Helper to create proto config for tests
astra::http2::ServerConfig make_config(const std::string& address, uint32_t port, uint32_t threads = 1) {
    astra::http2::ServerConfig config;
    config.set_address(address);
    config.set_port(port);
    config.set_thread_count(threads);
    return config;
}

} // namespace

TEST(Http2ServerTest, Construction) {
    auto server = std::make_unique<astra::http2::Server>(make_config("127.0.0.1", 9001));
    EXPECT_NE(server, nullptr);
}

TEST(Http2ServerTest, HandlerRegistration) {
    auto server = std::make_unique<astra::http2::Server>(make_config("127.0.0.1", 9002));
    
    server->handle("GET", "/test", [&](std::shared_ptr<router::IRequest>, std::shared_ptr<router::IResponse> res) {
        res->close();
    });
    
    // If we reached here without crash, it passed
    SUCCEED();
}

TEST(Http2ServerTest, MultipleHandlers) {
    auto server = std::make_unique<astra::http2::Server>(make_config("127.0.0.1", 9003));
    
    server->handle("GET", "/path1", [](auto, auto res) { res->close(); });
    server->handle("POST", "/path2", [](auto, auto res) { res->close(); });
    server->handle("GET", "/path3", [](auto, auto res) { res->close(); });
    
    SUCCEED();
}


TEST(Http2ServerTest, ThreadConfiguration) {
    EXPECT_NO_THROW({
        auto server1 = std::make_unique<astra::http2::Server>(make_config("127.0.0.1", 9004, 1));
        auto server2 = std::make_unique<astra::http2::Server>(make_config("127.0.0.1", 9005, 2));
        auto server4 = std::make_unique<astra::http2::Server>(make_config("127.0.0.1", 9006, 4));
    });
}

TEST(Http2ServerTest, BindToAllInterfaces) {
    auto server = std::make_unique<astra::http2::Server>(make_config("0.0.0.0", 9007));
    EXPECT_NE(server, nullptr);
}

TEST(Http2ServerTest, StressConstruction) {
    for(int i=0; i<100; ++i) {
        auto server = std::make_unique<astra::http2::Server>(make_config("127.0.0.1", 9008));
        EXPECT_NE(server, nullptr);
    }
}

class Http2ServerRuntimeTest : public Test {
protected:
    void SetUp() override {
        server_ = std::make_unique<astra::http2::Server>(make_config("127.0.0.1", 9009));
    }

    void TearDown() override {
        if (server_) {
            server_->stop();
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

    std::unique_ptr<astra::http2::Server> server_;
    std::thread server_thread_;
};

TEST_F(Http2ServerRuntimeTest, StartStop) {
    server_thread_ = std::thread([this]{
        server_->run();
    });
    
    // Wait for server to be ready (proper synchronization)
    server_->wait_until_ready();
    
    server_->stop();
    server_thread_.join();
    
    SUCCEED();
}
