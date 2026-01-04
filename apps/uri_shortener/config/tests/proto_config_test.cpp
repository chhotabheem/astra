/// @file proto_config_test.cpp
/// @brief TDD tests for protobuf config parsing - updated for new structure
/// @note Tests uri_shortener config with imported library definitions

#include "execution.pb.h"
#include "http2client.pb.h"
#include "http2server.pb.h"
#include "observability.pb.h"
#include "resilience.pb.h"
#include "uri_shortener.pb.h"

#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>
#include <gtest/gtest.h>

namespace uri_shortener::test {

// Global test environment for Protobuf cleanup
class ProtobufEnvironment : public ::testing::Environment {
public:
  ~ProtobufEnvironment() override = default;
  void TearDown() override {
    google::protobuf::ShutdownProtobufLibrary();
  }
};

[[maybe_unused]] static ::testing::Environment *const protobuf_env =
    ::testing::AddGlobalTestEnvironment(new ProtobufEnvironment);

// =============================================================================
// LIBRARY CONFIG TESTS - astra::http2::ServerConfig
// =============================================================================

TEST(Http2ServerConfigTest, Defaults) {
  astra::http2::ServerConfig server;
  EXPECT_EQ(server.address(), "");
  EXPECT_EQ(server.port(), 0);
  EXPECT_EQ(server.thread_count(), 0);
  EXPECT_EQ(server.max_connections(), 0);
  EXPECT_EQ(server.request_timeout_ms(), 0);
  EXPECT_EQ(server.max_concurrent_streams(), 0);
  EXPECT_EQ(server.initial_window_size(), 0);
}

TEST(Http2ServerConfigTest, CanSetAddress) {
  astra::http2::ServerConfig server;
  server.set_address("0.0.0.0");
  EXPECT_EQ(server.address(), "0.0.0.0");
}

TEST(Http2ServerConfigTest, CanSetPort) {
  astra::http2::ServerConfig server;
  server.set_port(8080);
  EXPECT_EQ(server.port(), 8080);
}

TEST(Http2ServerConfigTest, CanSetThreadCount) {
  astra::http2::ServerConfig server;
  server.set_thread_count(4);
  EXPECT_EQ(server.thread_count(), 4);
}

// =============================================================================
// LIBRARY CONFIG TESTS - astra::http2::ClientConfig
// =============================================================================

TEST(Http2ClientConfigTest, Defaults) {
  astra::http2::ClientConfig client;
  EXPECT_EQ(client.connect_timeout_ms(), 0);
  EXPECT_EQ(client.request_timeout_ms(), 0);
  EXPECT_EQ(client.max_concurrent_streams(), 0);
  EXPECT_EQ(client.initial_window_size(), 0);
}

TEST(Http2ClientConfigTest, CanSetTimeouts) {
  astra::http2::ClientConfig client;
  client.set_connect_timeout_ms(1000);
  client.set_request_timeout_ms(3000);
  EXPECT_EQ(client.connect_timeout_ms(), 1000);
  EXPECT_EQ(client.request_timeout_ms(), 3000);
}

TEST(Http2ClientConfigTest, CanSetStreamSettings) {
  astra::http2::ClientConfig client;
  client.set_max_concurrent_streams(100);
  client.set_initial_window_size(65535);
  EXPECT_EQ(client.max_concurrent_streams(), 100);
  EXPECT_EQ(client.initial_window_size(), 65535);
}

// =============================================================================
// LIBRARY CONFIG TESTS - execution::Config
// =============================================================================

TEST(ExecutionConfigTest, PoolExecutorDefaults) {
  execution::PoolExecutorConfig pe;
  EXPECT_EQ(pe.num_workers(), 0);
}

TEST(ExecutionConfigTest, CanSetPoolExecutorWorkers) {
  execution::PoolExecutorConfig pe;
  pe.set_num_workers(4);
  EXPECT_EQ(pe.num_workers(), 4);
}

TEST(ExecutionConfigTest, FullExecutionConfig) {
  execution::Config exec;
  exec.mutable_pool_executor()->set_num_workers(4);
  exec.mutable_affinity_executor()->set_num_lanes(2);

  EXPECT_EQ(exec.pool_executor().num_workers(), 4);
  EXPECT_EQ(exec.affinity_executor().num_lanes(), 2);
}

// =============================================================================
// LIBRARY CONFIG TESTS - observability::Config
// =============================================================================

TEST(ObservabilityConfigTest, Defaults) {
  observability::Config obs;
  EXPECT_EQ(obs.service_name(), "");
  EXPECT_EQ(obs.otlp_endpoint(), "");
  EXPECT_FALSE(obs.tracing_enabled());
  EXPECT_FALSE(obs.metrics_enabled());
}

TEST(ObservabilityConfigTest, CanSetServiceIdentity) {
  observability::Config obs;
  obs.set_service_name("uri-shortener");
  obs.set_service_version("1.0.0");
  obs.set_environment("development");
  EXPECT_EQ(obs.service_name(), "uri-shortener");
  EXPECT_EQ(obs.service_version(), "1.0.0");
}

TEST(ObservabilityConfigTest, CanEnableFeatures) {
  observability::Config obs;
  obs.set_tracing_enabled(true);
  obs.set_metrics_enabled(true);
  obs.set_logging_enabled(true);
  EXPECT_TRUE(obs.tracing_enabled());
  EXPECT_TRUE(obs.metrics_enabled());
}

// =============================================================================
// LIBRARY CONFIG TESTS - resilience::Config
// =============================================================================

TEST(ResilienceConfigTest, RetryPolicyDefaults) {
  resilience::RetryPolicy retry;
  EXPECT_EQ(retry.max_attempts(), 0);
  EXPECT_EQ(retry.initial_delay_ms(), 0);
}

TEST(ResilienceConfigTest, CanSetRetryPolicy) {
  resilience::RetryPolicy retry;
  retry.set_max_attempts(3);
  retry.set_initial_delay_ms(100);
  retry.set_max_delay_ms(2000);
  retry.set_backoff_multiplier(2.0);
  retry.add_retryable_status_codes(503);

  EXPECT_EQ(retry.max_attempts(), 3);
  EXPECT_EQ(retry.retryable_status_codes_size(), 1);
}

TEST(ResilienceConfigTest, CircuitBreakerDefaults) {
  resilience::CircuitBreakerPolicy cb;
  EXPECT_EQ(cb.failure_threshold(), 0);
  EXPECT_EQ(cb.success_threshold(), 0);
}

TEST(ResilienceConfigTest, CanSetCircuitBreaker) {
  resilience::CircuitBreakerPolicy cb;
  cb.set_failure_threshold(5);
  cb.set_success_threshold(2);
  cb.set_open_duration_ms(30000);

  EXPECT_EQ(cb.failure_threshold(), 5);
  EXPECT_EQ(cb.open_duration_ms(), 30000);
}

TEST(ResilienceConfigTest, LoadShedderDefaults) {
  resilience::LoadShedderPolicy ls;
  EXPECT_EQ(ls.max_concurrent_requests(), 0);
  EXPECT_EQ(ls.name(), "");
}

TEST(ResilienceConfigTest, CanSetLoadShedder) {
  resilience::LoadShedderPolicy ls;
  ls.set_max_concurrent_requests(1000);
  ls.set_name("uri-shortener");

  EXPECT_EQ(ls.max_concurrent_requests(), 1000);
  EXPECT_EQ(ls.name(), "uri-shortener");
}

// =============================================================================
// APP CONFIG TESTS - ServiceConfig
// =============================================================================

TEST(ServiceConfigTest, Defaults) {
  uri_shortener::ServiceConfig service;
  EXPECT_EQ(service.name(), "");
  EXPECT_EQ(service.environment(), "");
}

TEST(ServiceConfigTest, CanSetServiceName) {
  uri_shortener::ServiceConfig service;
  service.set_name("uri-shortener");
  EXPECT_EQ(service.name(), "uri-shortener");
}

// =============================================================================
// APP CONFIG TESTS - DataServiceClientConfig
// =============================================================================

TEST(DataServiceClientConfigTest, HasClientField) {
  uri_shortener::DataServiceClientConfig ds;
  EXPECT_FALSE(ds.has_client());
  ds.mutable_client();
  EXPECT_TRUE(ds.has_client());
}

TEST(DataServiceClientConfigTest, HasResilienceField) {
  uri_shortener::DataServiceClientConfig ds;
  EXPECT_FALSE(ds.has_resilience());
  ds.mutable_resilience();
  EXPECT_TRUE(ds.has_resilience());
}

TEST(DataServiceClientConfigTest, CanConfigureClient) {
  uri_shortener::DataServiceClientConfig ds;
  ds.mutable_client()->set_connect_timeout_ms(1000);
  ds.mutable_client()->set_request_timeout_ms(5000);

  EXPECT_EQ(ds.client().connect_timeout_ms(), 1000);
  EXPECT_EQ(ds.client().request_timeout_ms(), 5000);
}

// =============================================================================
// APP CONFIG TESTS - BootstrapConfig
// =============================================================================

TEST(BootstrapConfigTest, HasAllFields) {
  uri_shortener::BootstrapConfig bootstrap;
  EXPECT_FALSE(bootstrap.has_server());
  EXPECT_FALSE(bootstrap.has_execution());
  EXPECT_FALSE(bootstrap.has_observability());
  EXPECT_FALSE(bootstrap.has_dataservice());
  EXPECT_FALSE(bootstrap.has_service());
}

TEST(BootstrapConfigTest, CanSetServer) {
  uri_shortener::BootstrapConfig bootstrap;
  bootstrap.mutable_server()->set_address("0.0.0.0");
  bootstrap.mutable_server()->set_port(8080);
  bootstrap.mutable_server()->set_thread_count(2);

  EXPECT_EQ(bootstrap.server().address(), "0.0.0.0");
  EXPECT_EQ(bootstrap.server().port(), 8080);
}

TEST(BootstrapConfigTest, CanSetExecution) {
  uri_shortener::BootstrapConfig bootstrap;
  bootstrap.mutable_execution()->mutable_pool_executor()->set_num_workers(4);
  bootstrap.mutable_execution()->mutable_affinity_executor()->set_num_lanes(2);

  EXPECT_EQ(bootstrap.execution().pool_executor().num_workers(), 4);
  EXPECT_EQ(bootstrap.execution().affinity_executor().num_lanes(), 2);
}

TEST(BootstrapConfigTest, CanSetObservability) {
  uri_shortener::BootstrapConfig bootstrap;
  bootstrap.mutable_observability()->set_service_name("uri-shortener");
  bootstrap.mutable_observability()->set_tracing_enabled(true);

  EXPECT_EQ(bootstrap.observability().service_name(), "uri-shortener");
  EXPECT_TRUE(bootstrap.observability().tracing_enabled());
}

// =============================================================================
// APP CONFIG TESTS - RuntimeConfig
// =============================================================================

TEST(RuntimeConfigTest, HasLoadShedderField) {
  uri_shortener::RuntimeConfig runtime;
  EXPECT_FALSE(runtime.has_load_shedder());
  runtime.mutable_load_shedder();
  EXPECT_TRUE(runtime.has_load_shedder());
}

TEST(RuntimeConfigTest, CanSetLoadShedder) {
  uri_shortener::RuntimeConfig runtime;
  runtime.mutable_load_shedder()->set_max_concurrent_requests(10000);
  runtime.mutable_load_shedder()->set_name("uri-shortener");

  EXPECT_EQ(runtime.load_shedder().max_concurrent_requests(), 10000);
}

// =============================================================================
// APP CONFIG TESTS - Top-level Config
// =============================================================================

TEST(ConfigTest, CanCreateEmptyConfig) {
  uri_shortener::Config config;
  EXPECT_EQ(config.schema_version(), 0);
}

TEST(ConfigTest, CanSetSchemaVersion) {
  uri_shortener::Config config;
  config.set_schema_version(1);
  EXPECT_EQ(config.schema_version(), 1);
}

TEST(ConfigTest, HasBootstrapField) {
  uri_shortener::Config config;
  EXPECT_FALSE(config.has_bootstrap());
  config.mutable_bootstrap();
  EXPECT_TRUE(config.has_bootstrap());
}

TEST(ConfigTest, HasRuntimeField) {
  uri_shortener::Config config;
  EXPECT_FALSE(config.has_runtime());
  config.mutable_runtime();
  EXPECT_TRUE(config.has_runtime());
}

// =============================================================================
// JSON PARSING TESTS
// =============================================================================

TEST(JsonParsingTest, ParsesMinimalValidJson) {
  const char *json = R"({"schema_version": 1})";

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);

  EXPECT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(config.schema_version(), 1);
}

TEST(JsonParsingTest, ParsesServerConfig) {
  const char *json = R"({
        "bootstrap": {
            "server": {
                "address": "0.0.0.0",
                "port": 8080,
                "thread_count": 2,
                "max_connections": 1000,
                "request_timeout_ms": 5000
            }
        }
    })";

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);

  EXPECT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(config.bootstrap().server().address(), "0.0.0.0");
  EXPECT_EQ(config.bootstrap().server().port(), 8080);
}

TEST(JsonParsingTest, ParsesExecutionConfig) {
  const char *json = R"({
        "bootstrap": {
            "execution": {
                "pool_executor": {"num_workers": 4},
                "affinity_executor": {"num_lanes": 2}
            }
        }
    })";

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);

  EXPECT_TRUE(status.ok());
  EXPECT_EQ(config.bootstrap().execution().pool_executor().num_workers(), 4);
}

TEST(JsonParsingTest, ParsesObservabilityConfig) {
  const char *json = R"({
        "bootstrap": {
            "observability": {
                "service_name": "uri-shortener",
                "tracing_enabled": true,
                "metrics_enabled": true,
                "trace_sample_rate": 0.1
            }
        }
    })";

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);

  EXPECT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(config.bootstrap().observability().service_name(), "uri-shortener");
  EXPECT_TRUE(config.bootstrap().observability().tracing_enabled());
}

TEST(JsonParsingTest, ParsesDataserviceConfig) {
  const char *json = R"({
        "bootstrap": {
            "dataservice": {
                "client": {
                    "connect_timeout_ms": 1000,
                    "request_timeout_ms": 5000
                },
                "resilience": {
                    "retry": {"max_attempts": 3},
                    "circuit_breaker": {"failure_threshold": 5}
                }
            }
        }
    })";

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);

  EXPECT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(config.bootstrap().dataservice().client().connect_timeout_ms(),
            1000);
  EXPECT_EQ(
      config.bootstrap().dataservice().resilience().retry().max_attempts(), 3);
}

TEST(JsonParsingTest, ParsesRuntimeConfig) {
  const char *json = R"({
        "runtime": {
            "load_shedder": {
                "max_concurrent_requests": 10000,
                "name": "uri-shortener"
            }
        }
    })";

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);

  EXPECT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(config.runtime().load_shedder().max_concurrent_requests(), 10000);
}

TEST(JsonParsingTest, IgnoresUnknownFieldsWithOption) {
  const char *json = R"({
        "schema_version": 1,
        "unknown_field": "should be ignored"
    })";

  uri_shortener::Config config;
  google::protobuf::util::JsonParseOptions options;
  options.ignore_unknown_fields = true;

  auto status =
      google::protobuf::util::JsonStringToMessage(json, &config, options);
  EXPECT_TRUE(status.ok());
}

TEST(JsonParsingTest, FailsOnInvalidJson) {
  const char *json = "not valid json";

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);
  EXPECT_FALSE(status.ok());
}

TEST(JsonParsingTest, ParsesEmptyObject) {
  const char *json = "{}";

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(config.schema_version(), 0);
}

// =============================================================================
// MESSAGE DIFFERENCER TESTS
// =============================================================================

TEST(MessageDiffTest, IdenticalConfigsAreEqual) {
  uri_shortener::Config config1;
  config1.set_schema_version(1);
  config1.mutable_bootstrap()->mutable_server()->set_port(8080);

  uri_shortener::Config config2 = config1;

  EXPECT_TRUE(
      google::protobuf::util::MessageDifferencer::Equals(config1, config2));
}

TEST(MessageDiffTest, DifferentVersionsAreNotEqual) {
  uri_shortener::Config config1;
  config1.set_schema_version(1);

  uri_shortener::Config config2;
  config2.set_schema_version(2);

  EXPECT_FALSE(
      google::protobuf::util::MessageDifferencer::Equals(config1, config2));
}

// =============================================================================
// SERIALIZATION TESTS
// =============================================================================

TEST(SerializationTest, BinaryRoundTrip) {
  uri_shortener::Config original;
  original.set_schema_version(1);
  original.mutable_bootstrap()->mutable_server()->set_port(8080);
  original.mutable_runtime()
      ->mutable_load_shedder()
      ->set_max_concurrent_requests(10000);

  std::string binary;
  EXPECT_TRUE(original.SerializeToString(&binary));

  uri_shortener::Config parsed;
  EXPECT_TRUE(parsed.ParseFromString(binary));

  EXPECT_TRUE(
      google::protobuf::util::MessageDifferencer::Equals(original, parsed));
}

TEST(SerializationTest, JsonRoundTrip) {
  uri_shortener::Config original;
  original.set_schema_version(1);
  original.mutable_bootstrap()->mutable_server()->set_port(8080);

  std::string json;
  google::protobuf::util::MessageToJsonString(original, &json);

  uri_shortener::Config parsed;
  google::protobuf::util::JsonStringToMessage(json, &parsed);

  EXPECT_TRUE(
      google::protobuf::util::MessageDifferencer::Equals(original, parsed));
}

} // namespace uri_shortener::test
