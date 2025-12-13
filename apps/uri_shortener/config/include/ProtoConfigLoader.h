#pragma once
/// @file ProtoConfigLoader.h
/// @brief Loads and validates protobuf config from JSON files

#include "uri_shortener.pb.h"
#include <google/protobuf/util/json_util.h>
#include <string>
#include <fstream>
#include <sstream>
#include <optional>

namespace uri_shortener {

/// Result of loading config
struct ConfigLoadResult {
    bool success{false};
    Config config;
    std::string error;
    
    static ConfigLoadResult ok(Config cfg) {
        return {true, std::move(cfg), ""};
    }
    static ConfigLoadResult err(std::string msg) {
        return {false, Config{}, std::move(msg)};
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
        
        // Validate required fields
        auto validation_error = validate(config);
        if (validation_error) {
            return ConfigLoadResult::err(*validation_error);
        }
        
        return ConfigLoadResult::ok(std::move(config));
    }
    
private:
    /// Validate config, returns error message if invalid
    static std::optional<std::string> validate(const Config& config) {
        if (config.has_bootstrap()) {
            const auto& bootstrap = config.bootstrap();
            
            if (bootstrap.has_server()) {
                int port = bootstrap.server().port();
                if (port <= 0 || port > 65535) {
                    return "Invalid server.port: must be 1-65535";
                }
            }
            
            if (bootstrap.has_execution() && bootstrap.execution().has_shared_queue()) {
                if (bootstrap.execution().shared_queue().num_workers() <= 0) {
                    return "Invalid execution.shared_queue.num_workers: must be > 0";
                }
            }
            
            if (bootstrap.has_observability()) {
                double rate = bootstrap.observability().trace_sample_rate();
                if (rate < 0.0 || rate > 1.0) {
                    return "Invalid trace_sample_rate: must be 0.0-1.0";
                }
            }
        }
        
        return std::nullopt;
    }
};

} // namespace uri_shortener
