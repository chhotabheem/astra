#include <gtest/gtest.h>
#include "parsers/JsonConfigParser.h"
#include <fstream>
#include <sstream>

namespace config {

class JsonConfigParserTest : public ::testing::Test {
protected:
    std::string readTestConfigFile() {
        std::ifstream file(CMAKE_SOURCE_DIR "/libs/core/config/include/parsers/test_config.json");
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

TEST_F(JsonConfigParserTest, ParseValidJsonReturnsConfig) {
    JsonConfigParser parser;
    std::string json_content = readTestConfigFile();
    
    Config config = parser.parse(json_content);
    
    EXPECT_EQ(config.m_bootstrap.m_server.m_address, "127.0.0.1");
    EXPECT_EQ(config.m_bootstrap.m_server.m_port, 9090);
    EXPECT_EQ(config.m_bootstrap.m_threading.m_worker_threads, 4);
    EXPECT_EQ(config.m_bootstrap.m_threading.m_io_service_threads, 2);
    EXPECT_EQ(config.m_bootstrap.m_database.m_mongodb_uri, "mongodb://test:27017");
    EXPECT_EQ(config.m_bootstrap.m_database.m_redis_uri, "redis://test:6379");
    EXPECT_EQ(config.m_bootstrap.m_service.m_name, "json-test-service");
    EXPECT_EQ(config.m_bootstrap.m_service.m_environment, "testing");
}

TEST_F(JsonConfigParserTest, ParseOperationalConfig) {
    JsonConfigParser parser;
    std::string json_content = readTestConfigFile();
    
    Config config = parser.parse(json_content);
    
    EXPECT_EQ(config.m_operational.m_logging.m_level, "DEBUG");
    EXPECT_EQ(config.m_operational.m_logging.m_format, "text");
    EXPECT_FALSE(config.m_operational.m_logging.m_enable_access_logs);
    
    EXPECT_EQ(config.m_operational.m_timeouts.m_request_ms, 2000);
    EXPECT_EQ(config.m_operational.m_timeouts.m_database_ms, 1000);
    EXPECT_EQ(config.m_operational.m_timeouts.m_http_client_ms, 1500);
    
    EXPECT_EQ(config.m_operational.m_connection_pools.m_mongodb_pool_size, 20);
    EXPECT_EQ(config.m_operational.m_connection_pools.m_redis_pool_size, 10);
    EXPECT_EQ(config.m_operational.m_connection_pools.m_http2_max_connections, 200);
    
    EXPECT_FALSE(config.m_operational.m_observability.m_metrics_enabled);
    EXPECT_DOUBLE_EQ(config.m_operational.m_observability.m_tracing_sample_rate, 0.5);
}

TEST_F(JsonConfigParserTest, ParseRuntimeConfig) {
    JsonConfigParser parser;
    std::string json_content = readTestConfigFile();
    
    Config config = parser.parse(json_content);
    
    EXPECT_EQ(config.m_runtime.m_rate_limiting.m_global_rps_limit, 200000);
    EXPECT_EQ(config.m_runtime.m_rate_limiting.m_per_user_rps_limit, 2000);
    EXPECT_EQ(config.m_runtime.m_rate_limiting.m_burst_size, 10000);
    
    EXPECT_EQ(config.m_runtime.m_circuit_breaker.m_mongodb_threshold, 10);
    EXPECT_EQ(config.m_runtime.m_circuit_breaker.m_mongodb_timeout_sec, 60);
    EXPECT_EQ(config.m_runtime.m_circuit_breaker.m_redis_threshold, 5);
    EXPECT_EQ(config.m_runtime.m_circuit_breaker.m_redis_timeout_sec, 20);
    
    EXPECT_TRUE(config.m_runtime.m_feature_flags.m_enable_caching);
    EXPECT_FALSE(config.m_runtime.m_feature_flags.m_enable_url_preview);
    EXPECT_TRUE(config.m_runtime.m_feature_flags.m_compression_enabled);
    
    EXPECT_EQ(config.m_runtime.m_backpressure.m_worker_queue_max, 20000);
    EXPECT_EQ(config.m_runtime.m_backpressure.m_io_queue_max, 10000);
}

TEST_F(JsonConfigParserTest, ParseInvalidJsonThrows) {
    JsonConfigParser parser;
    std::string invalid_json = "{ invalid json }";
    
    EXPECT_THROW(parser.parse(invalid_json), std::runtime_error);
}

TEST_F(JsonConfigParserTest, ParseEmptyJsonThrows) {
    JsonConfigParser parser;
    std::string empty_json = "";
    
    EXPECT_THROW(parser.parse(empty_json), std::runtime_error);
}

TEST_F(JsonConfigParserTest, ParseMissingFieldsUsesDefaults) {
    JsonConfigParser parser;
    std::string json_with_minimal_data = R"({
        "bootstrap": {
            "server": {
                "address": "127.0.0.1"
            }
        }
    })";
    
    Config config = parser.parse(json_with_minimal_data);
    
    EXPECT_EQ(config.m_bootstrap.m_server.m_address, "127.0.0.1");
    EXPECT_EQ(config.m_bootstrap.m_server.m_port, 8080);
    EXPECT_EQ(config.m_operational.m_logging.m_level, "INFO");
}

}
