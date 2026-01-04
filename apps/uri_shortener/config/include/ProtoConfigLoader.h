#pragma once

#include "Result.h"
#include "uri_shortener.pb.h"

#include <cstdlib>
#include <fstream>
#include <google/protobuf/util/json_util.h>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace uri_shortener {

using ConfigResult = astra::outcome::Result<Config, std::string>;

class ProtoConfigLoader {
public:
  static ConfigResult loadFromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
      return ConfigResult::Err("Failed to open config file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromString(buffer.str());
  }

  static ConfigResult load() {
    return loadFromFile("config/uri_shortener.json");
  }

  static ConfigResult loadFromString(const std::string &json) {
    Config config;
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;

    auto status =
        google::protobuf::util::JsonStringToMessage(json, &config, options);
    if (!status.ok()) {
      return ConfigResult::Err("JSON parse error: " +
                               std::string(status.message()));
    }

    auto validation_error = validate(config);
    if (validation_error) {
      return ConfigResult::Err(*validation_error);
    }

    return ConfigResult::Ok(std::move(config));
  }

private:
  static std::optional<std::string> validate(const Config &config) {
    if (config.has_bootstrap()) {
      const auto &bootstrap = config.bootstrap();

      if (bootstrap.has_server()) {
        int port = bootstrap.server().port();
        if (port <= 0 || port > 65535) {
          return "Invalid server.port: must be 1-65535";
        }
      }

      if (bootstrap.has_execution() &&
          bootstrap.execution().has_pool_executor()) {
        if (bootstrap.execution().pool_executor().num_workers() <= 0) {
          return "Invalid execution.pool_executor.num_workers: must be > 0";
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
