#include <gtest/gtest.h>
#include "AppBuilder.h"
#include "InMemoryLinkRepository.h"
#include "RandomCodeGenerator.h"

namespace uri_shortener::test {

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

TEST(UriShortenerAppTest, Build_WithValidConfig_Succeeds) {
    auto config = makeValidConfig();
    
    auto result = AppBuilder(config)
        .domain()
        .backend()
        .messaging()
        .resilience()
        .build();
    
    EXPECT_TRUE(result.is_ok());
}

TEST(UriShortenerAppTest, Build_WithEmptyAddress_Fails) {
    auto config = makeValidConfig();
    config.mutable_bootstrap()->mutable_server()->set_address("");
    
    auto result = AppBuilder(config)
        .domain()
        .backend()
        .messaging()
        .resilience()
        .build();
    
    EXPECT_TRUE(result.is_err());
}

TEST(UriShortenerAppTest, Build_WithZeroPort_Fails) {
    auto config = makeValidConfig();
    config.mutable_bootstrap()->mutable_server()->set_port(0);
    
    auto result = AppBuilder(config)
        .domain()
        .backend()
        .messaging()
        .resilience()
        .build();
    
    EXPECT_TRUE(result.is_err());
}

TEST(UriShortenerAppTest, Build_WithMinimalConfig_Succeeds) {
    uri_shortener::Config config;
    config.set_schema_version(1);
    config.mutable_bootstrap()->mutable_server()->set_address("127.0.0.1");
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    auto result = AppBuilder(config)
        .domain()
        .backend()
        .messaging()
        .resilience()
        .build();
    
    EXPECT_TRUE(result.is_ok());
}

TEST(UriShortenerAppTest, Build_WithObservabilityConfig_Succeeds) {
    auto config = makeValidConfig();
    config.mutable_bootstrap()->mutable_observability()->set_metrics_enabled(true);
    config.mutable_bootstrap()->mutable_observability()->set_tracing_enabled(false);
    config.mutable_bootstrap()->mutable_observability()->set_otlp_endpoint("http://otel:4317");
    
    auto result = AppBuilder(config)
        .domain()
        .backend()
        .messaging()
        .resilience()
        .build();
    
    EXPECT_TRUE(result.is_ok());
}

} // namespace uri_shortener::test
