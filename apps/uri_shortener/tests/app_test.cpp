#include "InMemoryLinkRepository.h"
#include "RandomCodeGenerator.h"
#include "UriShortenerBuilder.h"

#include <gtest/gtest.h>

namespace uri_shortener::test {

uri_shortener::Config makeValidConfig() {
  uri_shortener::Config config;
  config.set_schema_version(1);
  config.mutable_bootstrap()->mutable_server()->set_uri("127.0.0.1:8080");
  config.mutable_bootstrap()
      ->mutable_execution()
      ->mutable_pool_executor()
      ->set_num_workers(2);
  config.mutable_bootstrap()->mutable_service()->set_name("uri-shortener-test");
  config.mutable_bootstrap()->mutable_service()->set_environment("test");
  return config;
}

TEST(UriShortenerAppTest, Build_WithValidConfig_Succeeds) {
  auto config = makeValidConfig();

  auto result = UriShortenerBuilder(config)
                    .domain()
                    .backend()
                    .messaging()
                    .resilience()
                    .build();

  EXPECT_TRUE(result.is_ok());
}

TEST(UriShortenerAppTest, Build_WithEmptyUri_Fails) {
  auto config = makeValidConfig();
  config.mutable_bootstrap()->mutable_server()->set_uri("");

  auto result = UriShortenerBuilder(config)
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
  config.mutable_bootstrap()->mutable_server()->set_uri("127.0.0.1:8080");

  auto result = UriShortenerBuilder(config)
                    .domain()
                    .backend()
                    .messaging()
                    .resilience()
                    .build();

  EXPECT_TRUE(result.is_ok());
}

TEST(UriShortenerAppTest, Build_WithObservabilityConfig_Succeeds) {
  auto config = makeValidConfig();
  config.mutable_bootstrap()->mutable_observability()->set_metrics_enabled(
      true);
  config.mutable_bootstrap()->mutable_observability()->set_tracing_enabled(
      false);
  config.mutable_bootstrap()->mutable_observability()->set_otlp_endpoint(
      "http://otel:4317");

  auto result = UriShortenerBuilder(config)
                    .domain()
                    .backend()
                    .messaging()
                    .resilience()
                    .build();

  EXPECT_TRUE(result.is_ok());
}

} // namespace uri_shortener::test
