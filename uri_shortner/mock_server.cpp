#include "Http1Server.h"
#include "Http2Server.h"
#include "UriService.h"
#include "UriController.h"
#include "IUriRepository.h"
#include <iostream>
#include <thread>
#include <csignal>
#include <unordered_map>
#include <mutex>

using namespace uri_shortener;

std::atomic<bool> running{true};

void signal_handler(int) {
    running = false;
}

// Thread-safe Mock Repository
class MockUriRepository : public IUriRepository {
public:
    uint64_t generate_id() override { 
        return current_id++; 
    }

    void save(const std::string& short_code, const std::string& long_url) override {
        std::lock_guard<std::mutex> lock(mutex_);
        store_[short_code] = long_url;
    }

    std::optional<std::string> find(const std::string& short_code) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = store_.find(short_code);
        if (it != store_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    std::atomic<uint64_t> current_id{1000};
    std::unordered_map<std::string, std::string> store_;
    std::mutex mutex_;
};

int main() {
    try {
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        std::cout << "Initializing Mock Infrastructure..." << std::endl;
        auto mock_repo = std::make_shared<MockUriRepository>();
        auto service = std::make_shared<UriService>(mock_repo);
        auto controller = std::make_shared<UriController>(service);

        auto setup_routes = [&](router::Router& r) {
            r.post("/shorten", [controller](router::IRequest& req, router::IResponse& res) {
                controller->shorten(req, res);
            });
            
            r.get("/:code", [controller](router::IRequest& req, router::IResponse& res) {
                controller->redirect(req, res);
            });
        };

        std::cout << "Starting Mock Servers..." << std::endl;
        http1::Server http1_server("0.0.0.0", 8081, 2);
        http2server::Server http2_server("0.0.0.0", "8080", 4);

        setup_routes(http1_server.router());
        setup_routes(http2_server.router());

        std::thread t1([&] { http1_server.run(); });
        std::thread t2([&] { http2_server.run(); });

        std::cout << "MOCK URI Shortener Service Running (No DB):" << std::endl;
        std::cout << "  - HTTP/2 (Traffic): http://localhost:8080" << std::endl;

        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cout << "Stopping servers..." << std::endl;
        http1_server.stop();
        http2_server.stop();
        
        if (t1.joinable()) t1.join();
        if (t2.joinable()) t2.join();

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
