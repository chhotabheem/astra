#include "Http2Client.h"
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <random>
#include <future>
#include <algorithm>
#include <numeric>
#include <iomanip>

using namespace http2client;

struct RequestStats {
    bool success = false;
    double latency_ms = 0.0;
    std::string type; // "SHORTEN" or "EXPAND"
};

struct ThreadStats {
    int success_shorten = 0;
    int success_expand = 0;
    int errors = 0;
    int timeouts = 0;
    std::vector<double> latencies; // Store all latencies for percentile calc
};

std::string random_string(size_t length) {
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    static thread_local std::mt19937 rng(std::random_device{}());
    static thread_local std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);
    
    std::string str(length, 0);
    for (size_t i = 0; i < length; ++i) {
        str[i] = charset[dist(rng)];
    }
    return str;
}

void worker(int id, int requests, int rps_per_thread, std::string host, std::string port, ThreadStats& stats) {
    (void)id;
    
    // Virtual User: Own Client Instance
    ClientConfig config;
    config.host = host;
    config.port = port;
    config.threads = 1; // 1 IO thread per VU is enough
    Client client(config);

    // Wait for connection
    int retries = 5;
    while (!client.is_connected() && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto delay = std::chrono::milliseconds(0);
    if (rps_per_thread > 0) {
        delay = std::chrono::milliseconds(1000 / rps_per_thread);
    }

    for (int i = 0; i < requests; ++i) {
        auto iter_start = std::chrono::steady_clock::now();
        std::string long_url = "http://example.com/" + random_string(10);
        
        // 1. Shorten
        auto t1 = std::chrono::high_resolution_clock::now();
        auto shorten_promise = std::make_shared<std::promise<std::string>>();
        auto shorten_future = shorten_promise->get_future();
        
        client.post("/shorten", long_url, [shorten_promise](const Response& res, const Error& err) {
            if (err || res.status_code() != 201) {
                shorten_promise->set_value("");
                return;
            }
            auto body = res.body();
            auto pos = body.find("short_code");
            if (pos != std::string::npos) {
                auto start = body.find("\"", pos + 12);
                auto end = body.find("\"", start + 1);
                if (start != std::string::npos && end != std::string::npos) {
                    shorten_promise->set_value(body.substr(start + 1, end - start - 1));
                } else {
                    shorten_promise->set_value("");
                }
            } else {
                shorten_promise->set_value("");
            }
        });

        if (shorten_future.wait_for(std::chrono::seconds(2)) == std::future_status::timeout) {
            stats.timeouts++;
            continue;
        }
        
        std::string short_code = shorten_future.get();
        auto t2 = std::chrono::high_resolution_clock::now();
        
        if (short_code.empty()) {
            stats.errors++;
        } else {
            stats.success_shorten++;
            stats.latencies.push_back(std::chrono::duration<double, std::milli>(t2 - t1).count());
        }

        if (short_code.empty()) continue;

        // 2. Expand
        auto t3 = std::chrono::high_resolution_clock::now();
        auto expand_promise = std::make_shared<std::promise<bool>>();
        auto expand_future = expand_promise->get_future();

        client.get("/" + short_code, [expand_promise](const Response& res, const Error& err) {
            if (err || res.status_code() != 302) {
                expand_promise->set_value(false);
            } else {
                expand_promise->set_value(true);
            }
        });

        if (expand_future.wait_for(std::chrono::seconds(2)) == std::future_status::timeout) {
            stats.timeouts++;
        } else {
            bool success = expand_future.get();
            auto t4 = std::chrono::high_resolution_clock::now();
            if (success) {
                stats.success_expand++;
                stats.latencies.push_back(std::chrono::duration<double, std::milli>(t4 - t3).count());
            } else {
                stats.errors++;
            }
        }

        // Pacing
        if (rps_per_thread > 0) {
            auto elapsed = std::chrono::steady_clock::now() - iter_start;
            if (elapsed < delay) {
                std::this_thread::sleep_for(delay - elapsed);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <total_requests> [concurrency] [rps]" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    std::string port = argv[2];
    int total_requests = std::stoi(argv[3]);
    int concurrency = (argc >= 5) ? std::stoi(argv[4]) : 10;
    int target_rps = (argc >= 6) ? std::stoi(argv[5]) : 0;
    
    if (concurrency <= 0) concurrency = 1;
    int requests_per_thread = total_requests / concurrency;
    int rps_per_thread = (target_rps > 0) ? (target_rps / concurrency) : 0;

    std::cout << "Starting Load Test (Virtual Users Mode):" << std::endl;
    std::cout << "  Total Requests: " << total_requests << std::endl;
    std::cout << "  Concurrency:    " << concurrency << " VUs" << std::endl;
    std::cout << "  Target RPS:     " << (target_rps > 0 ? std::to_string(target_rps) : "MAX (Burst)") << std::endl;

    std::vector<ThreadStats> all_stats(concurrency);
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < concurrency; ++i) {
        threads.emplace_back(worker, i, requests_per_thread, rps_per_thread, host, port, std::ref(all_stats[i]));
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    // Aggregation
    long total_shorten = 0;
    long total_expand = 0;
    long total_errors = 0;
    long total_timeouts = 0;
    std::vector<double> all_latencies;

    for (const auto& s : all_stats) {
        total_shorten += s.success_shorten;
        total_expand += s.success_expand;
        total_errors += s.errors;
        total_timeouts += s.timeouts;
        all_latencies.insert(all_latencies.end(), s.latencies.begin(), s.latencies.end());
    }

    std::sort(all_latencies.begin(), all_latencies.end());
    double min_lat = all_latencies.empty() ? 0 : all_latencies.front();
    double max_lat = all_latencies.empty() ? 0 : all_latencies.back();
    double avg_lat = all_latencies.empty() ? 0 : std::accumulate(all_latencies.begin(), all_latencies.end(), 0.0) / all_latencies.size();
    double p50 = all_latencies.empty() ? 0 : all_latencies[all_latencies.size() * 0.50];
    double p95 = all_latencies.empty() ? 0 : all_latencies[all_latencies.size() * 0.95];
    double p99 = all_latencies.empty() ? 0 : all_latencies[all_latencies.size() * 0.99];

    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Time:            " << std::fixed << std::setprecision(2) << diff.count() << "s" << std::endl;
    std::cout << "Actual RPS:      " << (total_requests / diff.count()) << std::endl;
    std::cout << "Shorten Success: " << total_shorten << std::endl;
    std::cout << "Expand Success:  " << total_expand << std::endl;
    std::cout << "Errors:          " << total_errors << std::endl;
    std::cout << "Timeouts:        " << total_timeouts << std::endl;
    
    std::cout << "\n=== Latency (ms) ===" << std::endl;
    std::cout << "Min: " << min_lat << std::endl;
    std::cout << "Avg: " << avg_lat << std::endl;
    std::cout << "Max: " << max_lat << std::endl;
    std::cout << "P50: " << p50 << std::endl;
    std::cout << "P95: " << p95 << std::endl;
    std::cout << "P99: " << p99 << std::endl;

    return 0;
}
