#include <gtest/gtest.h>
#include "AppBuilder.h"

namespace uri_shortener::test {

uri_shortener::Config makeBuilderTestConfig() {
    uri_shortener::Config config;
    config.set_schema_version(1);
    config.mutable_bootstrap()->mutable_server()->set_address("127.0.0.1");
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    config.mutable_bootstrap()->mutable_execution()->mutable_shared_queue()->set_num_workers(2);
    return config;
}

TEST(AppBuilderTest, Build_WithAllMethods_Succeeds) {
    auto config = makeBuilderTestConfig();
    
    auto result = AppBuilder(config)
        .domain()
        .backend()
        .messaging()
        .resilience()
        .build();
    
    EXPECT_TRUE(result.is_ok());
}

TEST(AppBuilderTest, Build_WithEmptyAddress_Fails) {
    auto config = makeBuilderTestConfig();
    config.mutable_bootstrap()->mutable_server()->set_address("");
    
    auto result = AppBuilder(config)
        .domain()
        .backend()
        .messaging()
        .resilience()
        .build();
    
    EXPECT_TRUE(result.is_err());
}

TEST(AppBuilderTest, DomainMethodChainsCorrectly) {
    auto config = makeBuilderTestConfig();
    
    AppBuilder builder(config);
    auto& returned = builder.domain();
    
    EXPECT_EQ(&returned, &builder);
}

TEST(AppBuilderTest, BackendMethodChainsCorrectly) {
    auto config = makeBuilderTestConfig();
    
    AppBuilder builder(config);
    builder.domain();
    auto& returned = builder.backend();
    
    EXPECT_EQ(&returned, &builder);
}

TEST(AppBuilderTest, MessagingMethodChainsCorrectly) {
    auto config = makeBuilderTestConfig();
    
    AppBuilder builder(config);
    builder.domain().backend();
    auto& returned = builder.messaging();
    
    EXPECT_EQ(&returned, &builder);
}

TEST(AppBuilderTest, ResilienceMethodChainsCorrectly) {
    auto config = makeBuilderTestConfig();
    
    AppBuilder builder(config);
    builder.domain().backend().messaging();
    auto& returned = builder.resilience();
    
    EXPECT_EQ(&returned, &builder);
}

} // namespace uri_shortener::test
