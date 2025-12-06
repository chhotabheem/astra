#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "UriService.h"
#include "IUriRepository.h"

using namespace testing;
using namespace uri_shortener;

class MockUriRepository : public IUriRepository {
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

TEST(UriServiceTest, Base62Encoding) {
    auto repo = std::make_shared<MockUriRepository>();
    UriService service(repo);

    std::string url = "http://example.com";
    std::string code = service.shorten(url);
    
    // ID 1000 -> Base62 "g8"
    EXPECT_EQ(code, "g8");
    EXPECT_EQ(service.expand("g8"), url);
}

TEST(UriServiceTest, MultipleUrls) {
    auto repo = std::make_shared<MockUriRepository>();
    UriService service(repo);

    std::string url1 = "http://example.com/1";
    std::string url2 = "http://example.com/2";

    std::string code1 = service.shorten(url1); // 1000 -> g8
    std::string code2 = service.shorten(url2); // 1001 -> h8 (assuming little endian or similar logic, let's just check uniqueness)

    EXPECT_NE(code1, code2);
    EXPECT_EQ(service.expand(code1), url1);
    EXPECT_EQ(service.expand(code2), url2);
}
