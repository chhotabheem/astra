#include "UriService.hpp"
#include "RedisUriRepository.hpp"
#include "MongoUriRepository.hpp"
#include <iostream>
#include <cassert>

// Mock Repository for unit testing Base62 logic without DB
class MockUriRepository : public uri_shortener::IUriRepository {
public:
    uint64_t generate_id() override { return current_id++; }
    void save(const std::string& short_code, const std::string& long_url) override {
        store[short_code] = long_url;
    }
    std::optional<std::string> find(const std::string& short_code) override {
        if (store.count(short_code)) return store[short_code];
        return std::nullopt;
    }

    uint64_t current_id = 1000;
    std::unordered_map<std::string, std::string> store;
};

void test_base62_encoding() {
    auto repo = std::make_shared<MockUriRepository>();
    uri_shortener::UriService service(repo);

    // Test 1: Shorten
    std::string url = "http://example.com";
    std::string code = service.shorten(url);
    
    // ID 1000 -> Base62 "g8" (16*62 + 8 = 1000)
    // 1000 / 62 = 16 rem 8 ('8')
    // 16 / 62 = 0 rem 16 ('g')
    // Result: "g8"
    
    assert(code == "g8");
    assert(service.expand("g8") == url);
    
    std::cout << "test_base62_encoding PASSED" << std::endl;
}

int main() {
    try {
        test_base62_encoding();
        std::cout << "All URI Shortener tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
