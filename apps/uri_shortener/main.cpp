/// @file main.cpp
/// @brief URI Shortener entry point - loads config from proto and starts app

#include "UriShortenerApp.h"
#include "ProtoConfigLoader.h"
#include <Provider.h>
#include <Log.h>
#include <cstdlib>

int main(int argc, char* argv[]) {
    // Bootstrap: minimal observability for startup errors
    // (Will be re-initialized with full config later in UriShortenerApp::create)
    obs::InitParams bootstrap_obs;
    bootstrap_obs.service_name = "uri-shortener";
    bootstrap_obs.service_version = "1.0.0";
    bootstrap_obs.environment = "bootstrap";
    obs::init(bootstrap_obs);

    // Determine config path
    std::string config_path = "config/default.json";
    if (argc > 1) {
        config_path = argv[1];
    }
    if (const char* env_config = std::getenv("URI_SHORTENER_CONFIG")) {
        config_path = env_config;
    }

    obs::info("Loading config", {{"path", config_path}});

    // Load proto config
    auto load_result = uri_shortener::ProtoConfigLoader::loadFromFile(config_path);
    if (!load_result.success) {
        obs::error("Failed to load config", 
            {{"path", config_path}, {"error", load_result.error}});
        return 1;
    }

    // Create app with proto config (this re-initializes obs with full config)
    auto result = url_shortener::UriShortenerApp::create(load_result.config);
    if (result.is_err()) {
        obs::error("Failed to start URI Shortener", {});
        return 1;
    }

    return result.value().run();
}
