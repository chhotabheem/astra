#include "parsers/JsonConfigParser.h"
#include <JsonDocument.h>
#include <stdexcept>

namespace config {

namespace {

void parseBootstrap(const json::JsonDocument& json, BootstrapConfig& config);
void parseOperational(const json::JsonDocument& json, OperationalConfig& config);
void parseRuntime(const json::JsonDocument& json, RuntimeConfig& config);

}

Config JsonConfigParser::parse(const std::string& raw_config) const {
    auto doc = json::JsonDocument::parse(raw_config);
    
    if (!doc.is_object()) {
        throw std::runtime_error("JSON root must be an object");
    }
    
    Config config;
    
    // Parse version (required)
    if (doc.contains("version")) {
        config.m_version = doc.get_int("version");
    }
    // If version is missing, default is 1 (set in struct)
    
    if (doc.contains("bootstrap")) {
        parseBootstrap(doc.get_child("bootstrap"), config.m_bootstrap);
    }
    
    if (doc.contains("operational")) {
        parseOperational(doc.get_child("operational"), config.m_operational);
    }
    
    if (doc.contains("runtime")) {
        parseRuntime(doc.get_child("runtime"), config.m_runtime);
    }
    
    return config;
}

namespace {

void parseBootstrap(const json::JsonDocument& json, BootstrapConfig& config) {
    if (json.contains("server")) {
        auto server = json.get_child("server");
        if (server.contains("address")) {
            config.m_server.m_address = server.get_string("address");
        }
        if (server.contains("port")) {
            config.m_server.m_port = static_cast<uint16_t>(server.get_int("port"));
        }
    }
    
    if (json.contains("threading")) {
        auto threading = json.get_child("threading");
        if (threading.contains("worker_threads")) {
            config.m_threading.m_worker_threads = threading.get_uint64("worker_threads");
        }
        if (threading.contains("io_service_threads")) {
            config.m_threading.m_io_service_threads = threading.get_uint64("io_service_threads");
        }
    }
    
    if (json.contains("database")) {
        auto database = json.get_child("database");
        if (database.contains("mongodb_uri")) {
            config.m_database.m_mongodb_uri = database.get_string("mongodb_uri");
        }
        if (database.contains("redis_uri")) {
            config.m_database.m_redis_uri = database.get_string("redis_uri");
        }
    }
    
    if (json.contains("service")) {
        auto service = json.get_child("service");
        if (service.contains("name")) {
            config.m_service.m_name = service.get_string("name");
        }
        if (service.contains("environment")) {
            config.m_service.m_environment = service.get_string("environment");
        }
    }
}

void parseOperational(const json::JsonDocument& json, OperationalConfig& config) {
    if (json.contains("logging")) {
        auto logging = json.get_child("logging");
        if (logging.contains("level")) {
            config.m_logging.m_level = logging.get_string("level");
        }
        if (logging.contains("format")) {
            config.m_logging.m_format = logging.get_string("format");
        }
        if (logging.contains("enable_access_logs")) {
            config.m_logging.m_enable_access_logs = logging.get_bool("enable_access_logs");
        }
    }
    
    if (json.contains("timeouts")) {
        auto timeouts = json.get_child("timeouts");
        if (timeouts.contains("request_ms")) {
            config.m_timeouts.m_request_ms = timeouts.get_int("request_ms");
        }
        if (timeouts.contains("database_ms")) {
            config.m_timeouts.m_database_ms = timeouts.get_int("database_ms");
        }
        if (timeouts.contains("http_client_ms")) {
            config.m_timeouts.m_http_client_ms = timeouts.get_int("http_client_ms");
        }
    }
    
    if (json.contains("connection_pools")) {
        auto pools = json.get_child("connection_pools");
        if (pools.contains("mongodb_pool_size")) {
            config.m_connection_pools.m_mongodb_pool_size = pools.get_uint64("mongodb_pool_size");
        }
        if (pools.contains("redis_pool_size")) {
            config.m_connection_pools.m_redis_pool_size = pools.get_uint64("redis_pool_size");
        }
        if (pools.contains("http2_max_connections")) {
            config.m_connection_pools.m_http2_max_connections = pools.get_uint64("http2_max_connections");
        }
    }
    
    if (json.contains("observability")) {
        auto obs = json.get_child("observability");
        if (obs.contains("metrics_enabled")) {
            config.m_observability.m_metrics_enabled = obs.get_bool("metrics_enabled");
        }
        if (obs.contains("tracing_sample_rate")) {
            config.m_observability.m_tracing_sample_rate = obs.get_double("tracing_sample_rate");
        }
    }
}

void parseRuntime(const json::JsonDocument& json, RuntimeConfig& config) {
    if (json.contains("rate_limiting")) {
        auto rate_limiting = json.get_child("rate_limiting");
        if (rate_limiting.contains("global_rps_limit")) {
            config.m_rate_limiting.m_global_rps_limit = rate_limiting.get_int("global_rps_limit");
        }
        if (rate_limiting.contains("per_user_rps_limit")) {
            config.m_rate_limiting.m_per_user_rps_limit = rate_limiting.get_int("per_user_rps_limit");
        }
        if (rate_limiting.contains("burst_size")) {
            config.m_rate_limiting.m_burst_size = rate_limiting.get_int("burst_size");
        }
    }
    
    if (json.contains("circuit_breaker")) {
        auto cb = json.get_child("circuit_breaker");
        if (cb.contains("mongodb_threshold")) {
            config.m_circuit_breaker.m_mongodb_threshold = cb.get_int("mongodb_threshold");
        }
        if (cb.contains("mongodb_timeout_sec")) {
            config.m_circuit_breaker.m_mongodb_timeout_sec = cb.get_int("mongodb_timeout_sec");
        }
        if (cb.contains("redis_threshold")) {
            config.m_circuit_breaker.m_redis_threshold = cb.get_int("redis_threshold");
        }
        if (cb.contains("redis_timeout_sec")) {
            config.m_circuit_breaker.m_redis_timeout_sec = cb.get_int("redis_timeout_sec");
        }
    }
    
    if (json.contains("feature_flags")) {
        auto flags = json.get_child("feature_flags");
        if (flags.contains("enable_caching")) {
            config.m_feature_flags.m_enable_caching = flags.get_bool("enable_caching");
        }
        if (flags.contains("enable_url_preview")) {
            config.m_feature_flags.m_enable_url_preview = flags.get_bool("enable_url_preview");
        }
        if (flags.contains("compression_enabled")) {
            config.m_feature_flags.m_compression_enabled = flags.get_bool("compression_enabled");
        }
    }
    
    if (json.contains("backpressure")) {
        auto backpressure = json.get_child("backpressure");
        if (backpressure.contains("worker_queue_max")) {
            config.m_backpressure.m_worker_queue_max = backpressure.get_uint64("worker_queue_max");
        }
        if (backpressure.contains("io_queue_max")) {
            config.m_backpressure.m_io_queue_max = backpressure.get_uint64("io_queue_max");
        }
    }
}

}

}
