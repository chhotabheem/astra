#include <gtest/gtest.h>
#include "ConfigStructs.h"

namespace config {

TEST(ServerConfigTest, DefaultValues) {
    ServerConfig server;
    EXPECT_EQ(server.m_address, "0.0.0.0");
    EXPECT_EQ(server.m_port, 8080);
}

TEST(ServerConfigTest, CustomValues) {
    ServerConfig server;
    server.m_address = "127.0.0.1";
    server.m_port = 9090;
    EXPECT_EQ(server.m_address, "127.0.0.1");
    EXPECT_EQ(server.m_port, 9090);
}

TEST(ThreadingConfigTest, DefaultValues) {
    ThreadingConfig threading;
    EXPECT_EQ(threading.m_worker_threads, 2);
    EXPECT_EQ(threading.m_io_service_threads, 1);
}

TEST(DatabaseConfigTest, EmptyByDefault) {
    DatabaseConfig database;
    EXPECT_TRUE(database.m_mongodb_uri.empty());
    EXPECT_TRUE(database.m_redis_uri.empty());
}

TEST(ServiceConfigTest, DefaultValues) {
    ServiceConfig service;
    EXPECT_EQ(service.m_name, "astra-service");
    EXPECT_EQ(service.m_environment, "development");
}

TEST(LoggingConfigTest, DefaultValues) {
    LoggingConfig logging;
    EXPECT_EQ(logging.m_level, "INFO");
    EXPECT_EQ(logging.m_format, "json");
    EXPECT_TRUE(logging.m_enable_access_logs);
}

TEST(TimeoutsConfigTest, DefaultValues) {
    TimeoutsConfig timeouts;
    EXPECT_EQ(timeouts.m_request_ms, 5000);
    EXPECT_EQ(timeouts.m_database_ms, 2000);
    EXPECT_EQ(timeouts.m_http_client_ms, 3000);
}

TEST(ConnectionPoolsConfigTest, DefaultValues) {
    ConnectionPoolsConfig pools;
    EXPECT_EQ(pools.m_mongodb_pool_size, 10);
    EXPECT_EQ(pools.m_redis_pool_size, 5);
    EXPECT_EQ(pools.m_http2_max_connections, 100);
}

TEST(ObservabilityConfigTest, DefaultValues) {
    ObservabilityConfig observability;
    EXPECT_TRUE(observability.m_metrics_enabled);
    EXPECT_DOUBLE_EQ(observability.m_tracing_sample_rate, 0.1);
}

TEST(RateLimitingConfigTest, DefaultValues) {
    RateLimitingConfig rate_limiting;
    EXPECT_EQ(rate_limiting.m_global_rps_limit, 100000);
    EXPECT_EQ(rate_limiting.m_per_user_rps_limit, 1000);
    EXPECT_EQ(rate_limiting.m_burst_size, 5000);
}

TEST(CircuitBreakerConfigTest, DefaultValues) {
    CircuitBreakerConfig circuit_breaker;
    EXPECT_EQ(circuit_breaker.m_mongodb_threshold, 5);
    EXPECT_EQ(circuit_breaker.m_mongodb_timeout_sec, 30);
    EXPECT_EQ(circuit_breaker.m_redis_threshold, 3);
    EXPECT_EQ(circuit_breaker.m_redis_timeout_sec, 30);
}

TEST(FeatureFlagsConfigTest, DefaultValues) {
    FeatureFlagsConfig feature_flags;
    EXPECT_TRUE(feature_flags.m_enable_caching);
    EXPECT_FALSE(feature_flags.m_enable_url_preview);
    EXPECT_TRUE(feature_flags.m_compression_enabled);
}

TEST(BackpressureConfigTest, DefaultValues) {
    BackpressureConfig backpressure;
    EXPECT_EQ(backpressure.m_worker_queue_max, 10000);
    EXPECT_EQ(backpressure.m_io_queue_max, 5000);
}

TEST(BootstrapConfigTest, ComposedStructs) {
    BootstrapConfig bootstrap;
    EXPECT_EQ(bootstrap.m_server.m_port, 8080);
    EXPECT_EQ(bootstrap.m_threading.m_worker_threads, 2);
    EXPECT_TRUE(bootstrap.m_database.m_mongodb_uri.empty());
    EXPECT_EQ(bootstrap.m_service.m_name, "astra-service");
}

TEST(OperationalConfigTest, ComposedStructs) {
    OperationalConfig operational;
    EXPECT_EQ(operational.m_logging.m_level, "INFO");
    EXPECT_EQ(operational.m_timeouts.m_request_ms, 5000);
    EXPECT_EQ(operational.m_connection_pools.m_mongodb_pool_size, 10);
    EXPECT_TRUE(operational.m_observability.m_metrics_enabled);
}

TEST(RuntimeConfigTest, ComposedStructs) {
    RuntimeConfig runtime;
    EXPECT_EQ(runtime.m_rate_limiting.m_global_rps_limit, 100000);
    EXPECT_EQ(runtime.m_circuit_breaker.m_mongodb_threshold, 5);
    EXPECT_TRUE(runtime.m_feature_flags.m_enable_caching);
    EXPECT_EQ(runtime.m_backpressure.m_worker_queue_max, 10000);
}

TEST(ConfigTest, FullComposition) {
    Config config;
    EXPECT_EQ(config.m_bootstrap.m_server.m_port, 8080);
    EXPECT_EQ(config.m_operational.m_logging.m_level, "INFO");
    EXPECT_EQ(config.m_runtime.m_rate_limiting.m_global_rps_limit, 100000);
}

}
