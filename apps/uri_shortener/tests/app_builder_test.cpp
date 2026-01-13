#include "UriShortenerBuilder.h"

#include <gtest/gtest.h>

namespace uri_shortener::test {

uri_shortener::Config makeBuilderTestConfig() {
  uri_shortener::Config config;
  config.set_schema_version(1);
  config.mutable_bootstrap()->mutable_server()->set_uri("127.0.0.1:8080");
  config.mutable_bootstrap()
      ->mutable_execution()
      ->mutable_pool_executor()
      ->set_num_workers(2);
  return config;
}

TEST(UriShortenerBuilderTest, Build_WithAllMethods_Succeeds) {
  auto config = makeBuilderTestConfig();

  auto result = UriShortenerBuilder(config)
                    .domain()
                    .backend()
                    .messaging()
                    .resilience()
                    .build();

  EXPECT_TRUE(result.is_ok());
}

TEST(UriShortenerBuilderTest, Build_WithEmptyUri_Fails) {
  auto config = makeBuilderTestConfig();
  config.mutable_bootstrap()->mutable_server()->set_uri("");

  auto result = UriShortenerBuilder(config)
                    .domain()
                    .backend()
                    .messaging()
                    .resilience()
                    .build();

  EXPECT_TRUE(result.is_err());
}

TEST(UriShortenerBuilderTest, DomainMethodChainsCorrectly) {
  auto config = makeBuilderTestConfig();

  UriShortenerBuilder builder(config);
  auto &returned = builder.domain();

  EXPECT_EQ(&returned, &builder);
}

TEST(UriShortenerBuilderTest, BackendMethodChainsCorrectly) {
  auto config = makeBuilderTestConfig();

  UriShortenerBuilder builder(config);
  builder.domain();
  auto &returned = builder.backend();

  EXPECT_EQ(&returned, &builder);
}

TEST(UriShortenerBuilderTest, MessagingMethodChainsCorrectly) {
  auto config = makeBuilderTestConfig();

  UriShortenerBuilder builder(config);
  builder.domain().backend();
  auto &returned = builder.messaging();

  EXPECT_EQ(&returned, &builder);
}

TEST(UriShortenerBuilderTest, ResilienceMethodChainsCorrectly) {
  auto config = makeBuilderTestConfig();

  UriShortenerBuilder builder(config);
  builder.domain().backend().messaging();
  auto &returned = builder.resilience();

  EXPECT_EQ(&returned, &builder);
}

} // namespace uri_shortener::test
