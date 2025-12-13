#pragma once
/// @file ProtoConfigLoader.h
/// @brief Loads and validates protobuf config from JSON files
/// @note App owns all config - this is an app-level utility, not library

#include "uri_shortener.pb.h"
#include "ConfigMigrator.h"
#include <google/protobuf/util/json_util.h>
#include <string>
#include <fstream>
#include <sstream>
#include <optional>

namespace uri_shortener {

/// Result of loading config with optional error message
struct ConfigLoadResult {
    bool success{false};
    Config config;
    std::string error;
    bool migrated{false};  // True if migration was applied
    
    static ConfigLoadResult ok(Config cfg, bool was_migrated = false) {
        return {true, std::move(cfg), "", was_migrated};
    }
    static ConfigLoadResult err(std::string msg) {
        return {false, Config{}, std::move(msg), false};
    }
};

/// Loads protobuf config from JSON files
class ProtoConfigLoader {
public:
    /// Load config from a JSON file path
    static ConfigLoadResult loadFromFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return ConfigLoadResult::err("Failed to open config file: " + path);
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return loadFromString(buffer.str());
    }
    
    /// Load config from a JSON string
    static ConfigLoadResult loadFromString(const std::string& json) {
        Config config;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = true;  // Forward compatibility
        
        auto status = google::protobuf::util::JsonStringToMessage(json, &config, options);
        if (!status.ok()) {
            return ConfigLoadResult::err("JSON parse error: " + std::string(status.message()));
        }
        
        // Auto-migrate to current version (industry pattern: Protobuf best practices)
        auto migration_result = ConfigMigrator::migrate(std::move(config));
        if (!migration_result.success) {
            return ConfigLoadResult::err("Migration error: " + migration_result.error);
        }
        config = std::move(migration_result.config);
        bool was_migrated = migration_result.migration_applied;
        
        // Validate required fields
        auto validation_error = validate(config);
        if (validation_error) {
            return ConfigLoadResult::err(*validation_error);
        }
        
        return ConfigLoadResult::ok(std::move(config), was_migrated);
    }
    
    /// Merge runtime config into existing config (for hot reload)
    static void mergeRuntime(Config& base, const Config& overlay) {
        if (overlay.has_runtime()) {
            base.mutable_runtime()->CopyFrom(overlay.runtime());
        }
    }
    
    /// Merge operational config into existing config (for hot reload)
    static void mergeOperational(Config& base, const Config& overlay) {
        if (overlay.has_operational()) {
            base.mutable_operational()->CopyFrom(overlay.operational());
        }
    }
    
private:
    /// Validate config, returns error message if invalid
    /// Note: schema_version is handled by ConfigMigrator before validation
    static std::optional<std::string> validate(const Config& config) {
        // Bootstrap validations
        if (config.has_bootstrap()) {
            const auto& bootstrap = config.bootstrap();
            
            if (bootstrap.has_server()) {
                int port = bootstrap.server().port();
                if (port <= 0 || port > 65535) {
                    return "Invalid server.port: must be 1-65535";
                }
            }
            
            if (bootstrap.has_threading()) {
                if (bootstrap.threading().worker_threads() <= 0) {
                    return "Invalid threading.worker_threads: must be > 0";
                }
                if (bootstrap.threading().io_service_threads() <= 0) {
                    return "Invalid threading.io_service_threads: must be > 0";
                }
            }
        }
        
        // Operational validations
        if (config.has_operational()) {
            const auto& op = config.operational();
            
            if (op.has_observability()) {
                double rate = op.observability().tracing_sample_rate();
                if (rate < 0.0 || rate > 1.0) {
                    return "Invalid tracing_sample_rate: must be 0.0-1.0";
                }
            }
            
            if (op.has_timeouts()) {
                if (op.timeouts().request_ms() <= 0) {
                    return "Invalid timeouts.request_ms: must be > 0";
                }
            }
        }
        
        return std::nullopt;  // Valid
    }
};

} // namespace uri_shortener

