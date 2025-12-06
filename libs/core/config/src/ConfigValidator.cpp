#include "ConfigValidator.h"
#include <set>

namespace config {

void ConfigValidator::validateBootstrap(const BootstrapConfig& config) {
    if (!isValidPort(config.m_server.m_port)) {
        throw std::invalid_argument("Invalid server port: " + std::to_string(config.m_server.m_port));
    }
    
    if (config.m_threading.m_worker_threads == 0) {
        throw std::invalid_argument("Worker threads must be greater than 0");
    }
    
    if (config.m_threading.m_io_service_threads == 0) {
        throw std::invalid_argument("IO service threads must be greater than 0");
    }
    
    if (config.m_database.m_mongodb_uri.empty()) {
        throw std::invalid_argument("MongoDB URI cannot be empty");
    }
    
    if (config.m_database.m_redis_uri.empty()) {
        throw std::invalid_argument("Redis URI cannot be empty");
    }
    
    if (config.m_service.m_name.empty()) {
        throw std::invalid_argument("Service name cannot be empty");
    }
}

std::optional<std::string> ConfigValidator::validateOperational(const OperationalConfig& config) {
    if (!isValidLogLevel(config.m_logging.m_level)) {
        return "Invalid log level: " + config.m_logging.m_level;
    }
    
    if (!isValidTimeout(config.m_timeouts.m_request_ms)) {
        return "Invalid request timeout: " + std::to_string(config.m_timeouts.m_request_ms);
    }
    
    if (!isValidTimeout(config.m_timeouts.m_database_ms)) {
        return "Invalid database timeout: " + std::to_string(config.m_timeouts.m_database_ms);
    }
    
    if (!isValidTimeout(config.m_timeouts.m_http_client_ms)) {
        return "Invalid HTTP client timeout: " + std::to_string(config.m_timeouts.m_http_client_ms);
    }
    
    if (config.m_connection_pools.m_mongodb_pool_size == 0) {
        return "MongoDB pool size must be greater than 0";
    }
    
    if (config.m_connection_pools.m_redis_pool_size == 0) {
        return "Redis pool size must be greater than 0";
    }
    
    if (config.m_connection_pools.m_http2_max_connections == 0) {
        return "HTTP2 max connections must be greater than 0";
    }
    
    if (config.m_observability.m_tracing_sample_rate < 0.0 || 
        config.m_observability.m_tracing_sample_rate > 1.0) {
        return "Tracing sample rate must be between 0.0 and 1.0";
    }
    
    return std::nullopt;
}

std::optional<std::string> ConfigValidator::validateRuntime(const RuntimeConfig& config) {
    if (config.m_rate_limiting.m_global_rps_limit < 0) {
        return "Global RPS rate limit cannot be negative";
    }
    
    if (config.m_rate_limiting.m_per_user_rps_limit < 0) {
        return "Per-user RPS rate limit cannot be negative";
    }
    
    if (config.m_rate_limiting.m_burst_size < 0) {
        return "Burst size cannot be negative";
    }
    
    if (config.m_circuit_breaker.m_mongodb_threshold <= 0) {
        return "Circuit breaker MongoDB threshold must be greater than 0";
    }
    
    if (config.m_circuit_breaker.m_redis_threshold <= 0) {
        return "Circuit breaker Redis threshold must be greater than 0";
    }
    
    if (config.m_backpressure.m_worker_queue_max == 0) {
        return "Worker queue max must be greater than 0";
    }
    
    if (config.m_backpressure.m_io_queue_max == 0) {
        return "IO queue max must be greater than 0";
    }
    
    return std::nullopt;
}

void ConfigValidator::validate(const Config& config) {
    // Validate schema version first
    if (config.m_version < 1 || config.m_version > 1) {
        throw std::invalid_argument("Unsupported config version: " + std::to_string(config.m_version) + 
                                    " (supported: 1)");
    }
    
    validateBootstrap(config.m_bootstrap);
    
    // Operational and runtime are fail-safe - we don't throw, just validate
    // Caller (ConfigProvider) can log warnings if needed
    auto operational_error = validateOperational(config.m_operational);
    if (operational_error.has_value()) {
        throw std::invalid_argument("Operational config validation failed: " + *operational_error);
    }
    
    auto runtime_error = validateRuntime(config.m_runtime);
    if (runtime_error.has_value()) {
        throw std::invalid_argument("Runtime config validation failed: " + *runtime_error);
    }
}

bool ConfigValidator::isValidLogLevel(const std::string& level) {
    static const std::set<std::string> valid_levels = {"DEBUG", "INFO", "WARN", "ERROR"};
    return valid_levels.find(level) != valid_levels.end();
}

bool ConfigValidator::isValidPort(uint16_t port) {
    return port > 0;
}

bool ConfigValidator::isValidTimeout(int timeout_ms) {
    return timeout_ms > 0;
}

}
