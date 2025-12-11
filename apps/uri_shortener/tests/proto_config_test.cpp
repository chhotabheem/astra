/// @file proto_config_test.cpp
/// @brief TDD tests for protobuf config parsing and validation
/// @note Part of 200 unique test cases for config library

#include <gtest/gtest.h>
#include "uri_shortener.pb.h"
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>

namespace uri_shortener::test {

// Global test environment for Protobuf cleanup (fixes Valgrind false positives)
class ProtobufEnvironment : public ::testing::Environment {
public:
    ~ProtobufEnvironment() override = default;
    void TearDown() override {
        google::protobuf::ShutdownProtobufLibrary();
    }
};

[[maybe_unused]] static ::testing::Environment* const protobuf_env =
    ::testing::AddGlobalTestEnvironment(new ProtobufEnvironment);


// =============================================================================
// PROTO GENERATION TESTS (verify protoc worked)
// =============================================================================

TEST(ProtoGenerationTest, CanCreateEmptyConfig) {
    Config config;
    EXPECT_EQ(config.schema_version(), 0);  // Proto3 default
}

TEST(ProtoGenerationTest, CanSetSchemaVersion) {
    Config config;
    config.set_schema_version(1);
    EXPECT_EQ(config.schema_version(), 1);
}

TEST(ProtoGenerationTest, HasBootstrapField) {
    Config config;
    EXPECT_FALSE(config.has_bootstrap());
    config.mutable_bootstrap();
    EXPECT_TRUE(config.has_bootstrap());
}

TEST(ProtoGenerationTest, HasOperationalField) {
    Config config;
    EXPECT_FALSE(config.has_operational());
    config.mutable_operational();
    EXPECT_TRUE(config.has_operational());
}

TEST(ProtoGenerationTest, HasRuntimeField) {
    Config config;
    EXPECT_FALSE(config.has_runtime());
    config.mutable_runtime();
    EXPECT_TRUE(config.has_runtime());
}

// =============================================================================
// BOOTSTRAP CONFIG TESTS
// =============================================================================

TEST(BootstrapConfigTest, ServerConfigDefaults) {
    ServerConfig server;
    EXPECT_EQ(server.address(), "");
    EXPECT_EQ(server.port(), 0);
}

TEST(BootstrapConfigTest, CanSetServerAddress) {
    ServerConfig server;
    server.set_address("0.0.0.0");
    EXPECT_EQ(server.address(), "0.0.0.0");
}

TEST(BootstrapConfigTest, CanSetServerPort) {
    ServerConfig server;
    server.set_port(8080);
    EXPECT_EQ(server.port(), 8080);
}

TEST(BootstrapConfigTest, ThreadingConfigDefaults) {
    ThreadingConfig threading;
    EXPECT_EQ(threading.worker_threads(), 0);
    EXPECT_EQ(threading.io_service_threads(), 0);
}

TEST(BootstrapConfigTest, CanSetWorkerThreads) {
    ThreadingConfig threading;
    threading.set_worker_threads(4);
    EXPECT_EQ(threading.worker_threads(), 4);
}

TEST(BootstrapConfigTest, CanSetIoServiceThreads) {
    ThreadingConfig threading;
    threading.set_io_service_threads(2);
    EXPECT_EQ(threading.io_service_threads(), 2);
}

TEST(BootstrapConfigTest, DatabaseConfigDefaults) {
    DatabaseConfig database;
    EXPECT_EQ(database.mongodb_uri(), "");
    EXPECT_EQ(database.redis_uri(), "");
}

TEST(BootstrapConfigTest, CanSetMongoDbUri) {
    DatabaseConfig database;
    database.set_mongodb_uri("mongodb://localhost:27017");
    EXPECT_EQ(database.mongodb_uri(), "mongodb://localhost:27017");
}

TEST(BootstrapConfigTest, CanSetRedisUri) {
    DatabaseConfig database;
    database.set_redis_uri("redis://localhost:6379");
    EXPECT_EQ(database.redis_uri(), "redis://localhost:6379");
}

TEST(BootstrapConfigTest, ServiceConfigDefaults) {
    ServiceConfig service;
    EXPECT_EQ(service.name(), "");
    EXPECT_EQ(service.environment(), "");
}

TEST(BootstrapConfigTest, CanSetServiceName) {
    ServiceConfig service;
    service.set_name("uri-shortener");
    EXPECT_EQ(service.name(), "uri-shortener");
}

TEST(BootstrapConfigTest, CanSetServiceEnvironment) {
    ServiceConfig service;
    service.set_environment("production");
    EXPECT_EQ(service.environment(), "production");
}

TEST(BootstrapConfigTest, FullBootstrapConfig) {
    BootstrapConfig bootstrap;
    bootstrap.mutable_server()->set_address("0.0.0.0");
    bootstrap.mutable_server()->set_port(8080);
    bootstrap.mutable_threading()->set_worker_threads(4);
    bootstrap.mutable_threading()->set_io_service_threads(2);
    bootstrap.mutable_database()->set_mongodb_uri("mongodb://localhost:27017");
    bootstrap.mutable_database()->set_redis_uri("redis://localhost:6379");
    bootstrap.mutable_service()->set_name("uri-shortener");
    bootstrap.mutable_service()->set_environment("development");
    
    EXPECT_EQ(bootstrap.server().address(), "0.0.0.0");
    EXPECT_EQ(bootstrap.server().port(), 8080);
    EXPECT_EQ(bootstrap.threading().worker_threads(), 4);
    EXPECT_EQ(bootstrap.service().name(), "uri-shortener");
}

// =============================================================================
// OPERATIONAL CONFIG TESTS
// =============================================================================

TEST(OperationalConfigTest, LoggingConfigDefaults) {
    LoggingConfig logging;
    EXPECT_EQ(logging.level(), "");
    EXPECT_EQ(logging.format(), "");
    EXPECT_EQ(logging.enable_access_logs(), false);
}

TEST(OperationalConfigTest, CanSetLogLevel) {
    LoggingConfig logging;
    logging.set_level("DEBUG");
    EXPECT_EQ(logging.level(), "DEBUG");
}

TEST(OperationalConfigTest, CanSetLogFormat) {
    LoggingConfig logging;
    logging.set_format("json");
    EXPECT_EQ(logging.format(), "json");
}

TEST(OperationalConfigTest, CanEnableAccessLogs) {
    LoggingConfig logging;
    logging.set_enable_access_logs(true);
    EXPECT_TRUE(logging.enable_access_logs());
}

TEST(OperationalConfigTest, TimeoutsConfigDefaults) {
    TimeoutsConfig timeouts;
    EXPECT_EQ(timeouts.request_ms(), 0);
    EXPECT_EQ(timeouts.database_ms(), 0);
    EXPECT_EQ(timeouts.http_client_ms(), 0);
}

TEST(OperationalConfigTest, CanSetRequestTimeout) {
    TimeoutsConfig timeouts;
    timeouts.set_request_ms(5000);
    EXPECT_EQ(timeouts.request_ms(), 5000);
}

TEST(OperationalConfigTest, CanSetDatabaseTimeout) {
    TimeoutsConfig timeouts;
    timeouts.set_database_ms(2000);
    EXPECT_EQ(timeouts.database_ms(), 2000);
}

TEST(OperationalConfigTest, CanSetHttpClientTimeout) {
    TimeoutsConfig timeouts;
    timeouts.set_http_client_ms(3000);
    EXPECT_EQ(timeouts.http_client_ms(), 3000);
}

TEST(OperationalConfigTest, ConnectionPoolsDefaults) {
    ConnectionPoolsConfig pools;
    EXPECT_EQ(pools.mongodb_pool_size(), 0);
    EXPECT_EQ(pools.redis_pool_size(), 0);
    EXPECT_EQ(pools.http2_max_connections(), 0);
}

TEST(OperationalConfigTest, CanSetMongoDbPoolSize) {
    ConnectionPoolsConfig pools;
    pools.set_mongodb_pool_size(10);
    EXPECT_EQ(pools.mongodb_pool_size(), 10);
}

TEST(OperationalConfigTest, CanSetRedisPoolSize) {
    ConnectionPoolsConfig pools;
    pools.set_redis_pool_size(5);
    EXPECT_EQ(pools.redis_pool_size(), 5);
}

TEST(OperationalConfigTest, CanSetHttp2MaxConnections) {
    ConnectionPoolsConfig pools;
    pools.set_http2_max_connections(100);
    EXPECT_EQ(pools.http2_max_connections(), 100);
}

TEST(OperationalConfigTest, ObservabilityConfigDefaults) {
    ObservabilityConfig obs;
    EXPECT_EQ(obs.metrics_enabled(), false);
    EXPECT_EQ(obs.tracing_enabled(), false);
    EXPECT_EQ(obs.logging_enabled(), false);
    EXPECT_DOUBLE_EQ(obs.tracing_sample_rate(), 0.0);
    EXPECT_EQ(obs.otlp_endpoint(), "");
    EXPECT_EQ(obs.service_version(), "");
}

TEST(OperationalConfigTest, CanEnableMetrics) {
    ObservabilityConfig obs;
    obs.set_metrics_enabled(true);
    EXPECT_TRUE(obs.metrics_enabled());
}

TEST(OperationalConfigTest, CanEnableTracing) {
    ObservabilityConfig obs;
    obs.set_tracing_enabled(true);
    EXPECT_TRUE(obs.tracing_enabled());
}

TEST(OperationalConfigTest, CanEnableLogging) {
    ObservabilityConfig obs;
    obs.set_logging_enabled(true);
    EXPECT_TRUE(obs.logging_enabled());
}

TEST(OperationalConfigTest, CanSetTracingSampleRate) {
    ObservabilityConfig obs;
    obs.set_tracing_sample_rate(0.1);
    EXPECT_DOUBLE_EQ(obs.tracing_sample_rate(), 0.1);
}

TEST(OperationalConfigTest, CanSetOtlpEndpoint) {
    ObservabilityConfig obs;
    obs.set_otlp_endpoint("http://localhost:4317");
    EXPECT_EQ(obs.otlp_endpoint(), "http://localhost:4317");
}

TEST(OperationalConfigTest, CanSetServiceVersion) {
    ObservabilityConfig obs;
    obs.set_service_version("1.0.0");
    EXPECT_EQ(obs.service_version(), "1.0.0");
}

// =============================================================================
// RUNTIME CONFIG TESTS
// =============================================================================

TEST(RuntimeConfigTest, RateLimitingDefaults) {
    RateLimitingConfig rate;
    EXPECT_EQ(rate.global_rps_limit(), 0);
    EXPECT_EQ(rate.per_user_rps_limit(), 0);
    EXPECT_EQ(rate.burst_size(), 0);
}

TEST(RuntimeConfigTest, CanSetGlobalRpsLimit) {
    RateLimitingConfig rate;
    rate.set_global_rps_limit(100000);
    EXPECT_EQ(rate.global_rps_limit(), 100000);
}

TEST(RuntimeConfigTest, CanSetPerUserRpsLimit) {
    RateLimitingConfig rate;
    rate.set_per_user_rps_limit(1000);
    EXPECT_EQ(rate.per_user_rps_limit(), 1000);
}

TEST(RuntimeConfigTest, CanSetBurstSize) {
    RateLimitingConfig rate;
    rate.set_burst_size(5000);
    EXPECT_EQ(rate.burst_size(), 5000);
}

TEST(RuntimeConfigTest, CircuitBreakerDefaults) {
    CircuitBreakerConfig cb;
    EXPECT_EQ(cb.mongodb_threshold(), 0);
    EXPECT_EQ(cb.mongodb_timeout_sec(), 0);
    EXPECT_EQ(cb.redis_threshold(), 0);
    EXPECT_EQ(cb.redis_timeout_sec(), 0);
}

TEST(RuntimeConfigTest, CanSetMongoDbThreshold) {
    CircuitBreakerConfig cb;
    cb.set_mongodb_threshold(5);
    EXPECT_EQ(cb.mongodb_threshold(), 5);
}

TEST(RuntimeConfigTest, CanSetMongoDbTimeoutSec) {
    CircuitBreakerConfig cb;
    cb.set_mongodb_timeout_sec(30);
    EXPECT_EQ(cb.mongodb_timeout_sec(), 30);
}

TEST(RuntimeConfigTest, CanSetRedisThreshold) {
    CircuitBreakerConfig cb;
    cb.set_redis_threshold(3);
    EXPECT_EQ(cb.redis_threshold(), 3);
}

TEST(RuntimeConfigTest, CanSetRedisTimeoutSec) {
    CircuitBreakerConfig cb;
    cb.set_redis_timeout_sec(30);
    EXPECT_EQ(cb.redis_timeout_sec(), 30);
}

TEST(RuntimeConfigTest, FeatureFlagsDefaults) {
    FeatureFlagsConfig flags;
    EXPECT_EQ(flags.enable_caching(), false);
    EXPECT_EQ(flags.enable_url_preview(), false);
    EXPECT_EQ(flags.compression_enabled(), false);
}

TEST(RuntimeConfigTest, CanEnableCaching) {
    FeatureFlagsConfig flags;
    flags.set_enable_caching(true);
    EXPECT_TRUE(flags.enable_caching());
}

TEST(RuntimeConfigTest, CanEnableUrlPreview) {
    FeatureFlagsConfig flags;
    flags.set_enable_url_preview(true);
    EXPECT_TRUE(flags.enable_url_preview());
}

TEST(RuntimeConfigTest, CanEnableCompression) {
    FeatureFlagsConfig flags;
    flags.set_compression_enabled(true);
    EXPECT_TRUE(flags.compression_enabled());
}

TEST(RuntimeConfigTest, BackpressureDefaults) {
    BackpressureConfig bp;
    EXPECT_EQ(bp.worker_queue_max(), 0);
    EXPECT_EQ(bp.io_queue_max(), 0);
}

TEST(RuntimeConfigTest, CanSetWorkerQueueMax) {
    BackpressureConfig bp;
    bp.set_worker_queue_max(10000);
    EXPECT_EQ(bp.worker_queue_max(), 10000);
}

TEST(RuntimeConfigTest, CanSetIoQueueMax) {
    BackpressureConfig bp;
    bp.set_io_queue_max(5000);
    EXPECT_EQ(bp.io_queue_max(), 5000);
}

// =============================================================================
// JSON PARSING TESTS
// =============================================================================

TEST(JsonParsingTest, ParsesMinimalValidJson) {
    const char* json = R"({"schema_version": 1})";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    
    EXPECT_TRUE(status.ok()) << status.message();
    EXPECT_EQ(config.schema_version(), 1);
}

TEST(JsonParsingTest, ParsesServerConfig) {
    const char* json = R"({
        "schema_version": 1,
        "bootstrap": {
            "server": {"address": "127.0.0.1", "port": 9000}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    
    EXPECT_TRUE(status.ok()) << status.message();
    EXPECT_EQ(config.bootstrap().server().address(), "127.0.0.1");
    EXPECT_EQ(config.bootstrap().server().port(), 9000);
}

TEST(JsonParsingTest, ParsesThreadingConfig) {
    const char* json = R"({
        "bootstrap": {
            "threading": {"worker_threads": 8, "io_service_threads": 4}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.bootstrap().threading().worker_threads(), 8);
    EXPECT_EQ(config.bootstrap().threading().io_service_threads(), 4);
}

TEST(JsonParsingTest, ParsesDatabaseConfig) {
    const char* json = R"({
        "bootstrap": {
            "database": {
                "mongodb_uri": "mongodb://db.example.com:27017",
                "redis_uri": "redis://cache.example.com:6379"
            }
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.bootstrap().database().mongodb_uri(), "mongodb://db.example.com:27017");
    EXPECT_EQ(config.bootstrap().database().redis_uri(), "redis://cache.example.com:6379");
}

TEST(JsonParsingTest, ParsesLoggingConfig) {
    const char* json = R"({
        "operational": {
            "logging": {"level": "DEBUG", "format": "text", "enable_access_logs": true}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.operational().logging().level(), "DEBUG");
    EXPECT_EQ(config.operational().logging().format(), "text");
    EXPECT_TRUE(config.operational().logging().enable_access_logs());
}

TEST(JsonParsingTest, ParsesTimeoutsConfig) {
    const char* json = R"({
        "operational": {
            "timeouts": {"request_ms": 10000, "database_ms": 5000, "http_client_ms": 8000}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.operational().timeouts().request_ms(), 10000);
    EXPECT_EQ(config.operational().timeouts().database_ms(), 5000);
    EXPECT_EQ(config.operational().timeouts().http_client_ms(), 8000);
}

TEST(JsonParsingTest, ParsesRateLimitingConfig) {
    const char* json = R"({
        "runtime": {
            "rate_limiting": {"global_rps_limit": 50000, "per_user_rps_limit": 500, "burst_size": 2500}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.runtime().rate_limiting().global_rps_limit(), 50000);
    EXPECT_EQ(config.runtime().rate_limiting().per_user_rps_limit(), 500);
    EXPECT_EQ(config.runtime().rate_limiting().burst_size(), 2500);
}

TEST(JsonParsingTest, ParsesCircuitBreakerConfig) {
    const char* json = R"({
        "runtime": {
            "circuit_breaker": {
                "mongodb_threshold": 10,
                "mongodb_timeout_sec": 60,
                "redis_threshold": 5,
                "redis_timeout_sec": 45
            }
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.runtime().circuit_breaker().mongodb_threshold(), 10);
    EXPECT_EQ(config.runtime().circuit_breaker().mongodb_timeout_sec(), 60);
    EXPECT_EQ(config.runtime().circuit_breaker().redis_threshold(), 5);
    EXPECT_EQ(config.runtime().circuit_breaker().redis_timeout_sec(), 45);
}

TEST(JsonParsingTest, ParsesFeatureFlagsConfig) {
    const char* json = R"({
        "runtime": {
            "feature_flags": {
                "enable_caching": false,
                "enable_url_preview": true,
                "compression_enabled": false
            }
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    
    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(config.runtime().feature_flags().enable_caching());
    EXPECT_TRUE(config.runtime().feature_flags().enable_url_preview());
    EXPECT_FALSE(config.runtime().feature_flags().compression_enabled());
}

TEST(JsonParsingTest, ParsesObservabilityConfig) {
    const char* json = R"({
        "operational": {
            "observability": {
                "metrics_enabled": true,
                "tracing_enabled": true,
                "logging_enabled": false,
                "tracing_sample_rate": 0.5,
                "otlp_endpoint": "http://otel-collector:4317",
                "service_version": "2.0.0"
            }
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    
    EXPECT_TRUE(status.ok());
    EXPECT_TRUE(config.operational().observability().metrics_enabled());
    EXPECT_TRUE(config.operational().observability().tracing_enabled());
    EXPECT_FALSE(config.operational().observability().logging_enabled());
    EXPECT_DOUBLE_EQ(config.operational().observability().tracing_sample_rate(), 0.5);
    EXPECT_EQ(config.operational().observability().otlp_endpoint(), "http://otel-collector:4317");
    EXPECT_EQ(config.operational().observability().service_version(), "2.0.0");
}

TEST(JsonParsingTest, IgnoresUnknownFieldsWithOption) {
    const char* json = R"({
        "schema_version": 1,
        "unknown_field": "should be ignored"
    })";
    
    Config config;
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;
    
    auto status = google::protobuf::util::JsonStringToMessage(json, &config, options);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.schema_version(), 1);
}

TEST(JsonParsingTest, FailsOnUnknownFieldsWithoutOption) {
    const char* json = R"({
        "schema_version": 1,
        "unknown_field": "should fail"
    })";
    
    Config config;
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = false;
    
    auto status = google::protobuf::util::JsonStringToMessage(json, &config, options);
    EXPECT_FALSE(status.ok());
}

TEST(JsonParsingTest, FailsOnInvalidJson) {
    const char* json = "not valid json";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_FALSE(status.ok());
}

TEST(JsonParsingTest, FailsOnEmptyString) {
    const char* json = "";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_FALSE(status.ok());
}

TEST(JsonParsingTest, ParsesEmptyObject) {
    const char* json = "{}";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.schema_version(), 0);
}

TEST(JsonParsingTest, ParsesFullConfig) {
    const char* json = R"({
        "schema_version": 1,
        "bootstrap": {
            "server": {"address": "0.0.0.0", "port": 8080},
            "threading": {"worker_threads": 4, "io_service_threads": 2},
            "database": {"mongodb_uri": "mongodb://localhost:27017", "redis_uri": "redis://localhost:6379"},
            "service": {"name": "uri-shortener", "environment": "development"}
        },
        "operational": {
            "logging": {"level": "INFO", "format": "json", "enable_access_logs": true},
            "timeouts": {"request_ms": 5000, "database_ms": 2000, "http_client_ms": 3000},
            "connection_pools": {"mongodb_pool_size": 10, "redis_pool_size": 5, "http2_max_connections": 100},
            "observability": {
                "metrics_enabled": true, "tracing_enabled": true, "logging_enabled": true,
                "tracing_sample_rate": 0.1, "otlp_endpoint": "http://localhost:4317", "service_version": "1.0.0"
            }
        },
        "runtime": {
            "rate_limiting": {"global_rps_limit": 100000, "per_user_rps_limit": 1000, "burst_size": 5000},
            "circuit_breaker": {"mongodb_threshold": 5, "mongodb_timeout_sec": 30, "redis_threshold": 3, "redis_timeout_sec": 30},
            "feature_flags": {"enable_caching": true, "enable_url_preview": false, "compression_enabled": true},
            "backpressure": {"worker_queue_max": 10000, "io_queue_max": 5000}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    
    EXPECT_TRUE(status.ok()) << status.message();
    EXPECT_EQ(config.schema_version(), 1);
    EXPECT_EQ(config.bootstrap().server().port(), 8080);
    EXPECT_EQ(config.operational().timeouts().request_ms(), 5000);
    EXPECT_EQ(config.runtime().rate_limiting().global_rps_limit(), 100000);
}

// =============================================================================
// MESSAGE DIFFERENCER TESTS (for semantic diff)
// =============================================================================

TEST(MessageDiffTest, IdenticalConfigsAreEqual) {
    Config config1;
    config1.set_schema_version(1);
    config1.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    Config config2 = config1;
    
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(config1, config2));
}

TEST(MessageDiffTest, DifferentVersionsAreNotEqual) {
    Config config1;
    config1.set_schema_version(1);
    
    Config config2;
    config2.set_schema_version(2);
    
    EXPECT_FALSE(google::protobuf::util::MessageDifferencer::Equals(config1, config2));
}

TEST(MessageDiffTest, DifferentPortsAreNotEqual) {
    Config config1;
    config1.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    Config config2;
    config2.mutable_bootstrap()->mutable_server()->set_port(9000);
    
    EXPECT_FALSE(google::protobuf::util::MessageDifferencer::Equals(config1, config2));
}

TEST(MessageDiffTest, DifferencerReportsDifferences) {
    Config config1;
    config1.set_schema_version(1);
    config1.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    Config config2;
    config2.set_schema_version(1);
    config2.mutable_bootstrap()->mutable_server()->set_port(9000);
    
    std::string diff_report;
    google::protobuf::util::MessageDifferencer differencer;
    differencer.ReportDifferencesToString(&diff_report);
    
    EXPECT_FALSE(differencer.Compare(config1, config2));
    EXPECT_FALSE(diff_report.empty());
}

// =============================================================================
// SERIALIZATION ROUND-TRIP TESTS
// =============================================================================

TEST(SerializationTest, BinaryRoundTrip) {
    Config original;
    original.set_schema_version(1);
    original.mutable_bootstrap()->mutable_server()->set_port(8080);
    original.mutable_runtime()->mutable_rate_limiting()->set_global_rps_limit(100000);
    
    std::string binary;
    EXPECT_TRUE(original.SerializeToString(&binary));
    
    Config parsed;
    EXPECT_TRUE(parsed.ParseFromString(binary));
    
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(original, parsed));
}

TEST(SerializationTest, JsonRoundTrip) {
    Config original;
    original.set_schema_version(1);
    original.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    std::string json;
    google::protobuf::util::MessageToJsonString(original, &json);
    
    Config parsed;
    google::protobuf::util::JsonStringToMessage(json, &parsed);
    
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(original, parsed));
}

} // namespace uri_shortener::test
