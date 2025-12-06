#pragma once
#include <string>
#include <cstdint>
#include <cstddef>

namespace config {

struct ServerConfig {
    std::string m_address{"0.0.0.0"};
    uint16_t m_port{8080};
};

struct ThreadingConfig {
    size_t m_worker_threads{2};
    size_t m_io_service_threads{1};
};

struct DatabaseConfig {
    std::string m_mongodb_uri;
    std::string m_redis_uri;
};

struct ServiceConfig {
    std::string m_name{"astra-service"};
    std::string m_environment{"development"};
};

struct LoggingConfig {
    std::string m_level{"INFO"};
    std::string m_format{"json"};
    bool m_enable_access_logs{true};
};

struct TimeoutsConfig {
    int m_request_ms{5000};
    int m_database_ms{2000};
    int m_http_client_ms{3000};
};

struct ConnectionPoolsConfig {
    size_t m_mongodb_pool_size{10};
    size_t m_redis_pool_size{5};
    size_t m_http2_max_connections{100};
};

struct ObservabilityConfig {
    bool m_metrics_enabled{true};
    double m_tracing_sample_rate{0.1};
};

struct RateLimitingConfig {
    int m_global_rps_limit{100000};
    int m_per_user_rps_limit{1000};
    int m_burst_size{5000};
};

struct CircuitBreakerConfig {
    int m_mongodb_threshold{5};
    int m_mongodb_timeout_sec{30};
    int m_redis_threshold{3};
    int m_redis_timeout_sec{30};
};

struct FeatureFlagsConfig {
    bool m_enable_caching{true};
    bool m_enable_url_preview{false};
    bool m_compression_enabled{true};
};

struct BackpressureConfig {
    size_t m_worker_queue_max{10000};
    size_t m_io_queue_max{5000};
};

struct BootstrapConfig {
    ServerConfig m_server;
    ThreadingConfig m_threading;
    DatabaseConfig m_database;
    ServiceConfig m_service;
};

struct OperationalConfig {
    LoggingConfig m_logging;
    TimeoutsConfig m_timeouts;
    ConnectionPoolsConfig m_connection_pools;
    ObservabilityConfig m_observability;
};

struct RuntimeConfig {
    RateLimitingConfig m_rate_limiting;
    CircuitBreakerConfig m_circuit_breaker;
    FeatureFlagsConfig m_feature_flags;
    BackpressureConfig m_backpressure;
};

struct Config {
    int m_version{1};  // Schema version - always validate
    BootstrapConfig m_bootstrap;
    OperationalConfig m_operational;
    RuntimeConfig m_runtime;
};

}
