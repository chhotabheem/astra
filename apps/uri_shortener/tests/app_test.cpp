/// @file test_app.cpp
/// @brief TDD tests for UriShortenerApp with proto config

#include <gtest/gtest.h>
#include "UriShortenerApp.h"
#include "InMemoryLinkRepository.h"
#include "RandomCodeGenerator.h"

namespace url_shortener::test {

// Helper to create a valid proto config
uri_shortener::Config makeValidConfig() {
    uri_shortener::Config config;
    config.set_schema_version(1);
    config.mutable_bootstrap()->mutable_server()->set_address("127.0.0.1");
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    config.mutable_bootstrap()->mutable_execution()->mutable_shared_queue()->set_num_workers(2);
    config.mutable_bootstrap()->mutable_service()->set_name("uri-shortener-test");
    config.mutable_bootstrap()->mutable_service()->set_environment("test");
    return config;
}

// =============================================================================
// Factory Tests
// =============================================================================

TEST(UriShortenerAppTest, Create_WithValidConfig_Succeeds) {
    auto config = makeValidConfig();
    
    auto result = UriShortenerApp::create(config);
    
    EXPECT_TRUE(result.is_ok());
}

TEST(UriShortenerAppTest, Create_WithEmptyAddress_Fails) {
    auto config = makeValidConfig();
    config.mutable_bootstrap()->mutable_server()->set_address("");
    
    auto result = UriShortenerApp::create(config);
    
    EXPECT_TRUE(result.is_err());
}

TEST(UriShortenerAppTest, Create_WithZeroPort_Fails) {
    auto config = makeValidConfig();
    config.mutable_bootstrap()->mutable_server()->set_port(0);
    
    auto result = UriShortenerApp::create(config);
    
    EXPECT_TRUE(result.is_err());
}

TEST(UriShortenerAppTest, Create_WithMinimalConfig_Succeeds) {
    uri_shortener::Config config;
    config.set_schema_version(1);
    config.mutable_bootstrap()->mutable_server()->set_address("127.0.0.1");
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    auto result = UriShortenerApp::create(config);
    
    EXPECT_TRUE(result.is_ok());
}

// =============================================================================
// Dependency Injection Tests
// =============================================================================

TEST(UriShortenerAppTest, Create_WithCustomRepository_UsesIt) {
    auto repo = std::make_shared<infrastructure::InMemoryLinkRepository>();
    auto gen = std::make_shared<infrastructure::RandomCodeGenerator>();
    
    UriShortenerApp::Overrides overrides;
    overrides.repository = repo;
    overrides.code_generator = gen;
    
    auto config = makeValidConfig();
    
    auto result = UriShortenerApp::create(config, overrides);
    
    EXPECT_TRUE(result.is_ok());
}

// =============================================================================
// Observability Config Tests
// =============================================================================

TEST(UriShortenerAppTest, Create_WithObservabilityConfig_Succeeds) {
    auto config = makeValidConfig();
    config.mutable_bootstrap()->mutable_observability()->set_metrics_enabled(true);
    config.mutable_bootstrap()->mutable_observability()->set_tracing_enabled(false);
    config.mutable_bootstrap()->mutable_observability()->set_otlp_endpoint("http://otel:4317");
    
    auto result = UriShortenerApp::create(config);
    
    EXPECT_TRUE(result.is_ok());
}

} // namespace url_shortener::test
