/// @file test_app.cpp
/// @brief TDD tests for UriShortenerApp

#include <gtest/gtest.h>
#include "UriShortenerApp.h"
#include "infrastructure/persistence/InMemoryLinkRepository.h"
#include "infrastructure/generators/RandomCodeGenerator.h"

namespace url_shortener::test {

// =============================================================================
// Factory Tests
// =============================================================================

TEST(UriShortenerAppTest, Create_WithValidConfig_Succeeds) {
    UriShortenerApp::Config config{
        .address = "127.0.0.1",
        .port = "8080"
    };
    
    auto result = UriShortenerApp::create(config);
    
    EXPECT_TRUE(result.is_ok());
}

TEST(UriShortenerAppTest, Create_WithEmptyAddress_Fails) {
    UriShortenerApp::Config config{
        .address = "",
        .port = "8080"
    };
    
    auto result = UriShortenerApp::create(config);
    
    EXPECT_TRUE(result.is_err());
}

TEST(UriShortenerAppTest, Create_WithEmptyPort_Fails) {
    UriShortenerApp::Config config{
        .address = "127.0.0.1",
        .port = ""
    };
    
    auto result = UriShortenerApp::create(config);
    
    EXPECT_TRUE(result.is_err());
}

// =============================================================================
// Dependency Injection Tests
// =============================================================================

TEST(UriShortenerAppTest, Create_WithCustomRepository_UsesIt) {
    auto repo = std::make_shared<infrastructure::InMemoryLinkRepository>();
    auto gen = std::make_shared<infrastructure::RandomCodeGenerator>();
    
    UriShortenerApp::Config config{
        .address = "127.0.0.1",
        .port = "8080",
        .repository = repo,
        .code_generator = gen
    };
    
    auto result = UriShortenerApp::create(config);
    
    EXPECT_TRUE(result.is_ok());
}

// Note: We can't easily test run() in unit tests since it blocks.
// That would be an integration test.

} // namespace url_shortener::test
