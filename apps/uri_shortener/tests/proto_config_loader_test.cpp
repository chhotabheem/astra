/// @file proto_config_loader_test.cpp
/// @brief TDD tests for ProtoConfigLoader

#include <gtest/gtest.h>
#include "ProtoConfigLoader.h"
#include <fstream>
#include <cstdio>

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
// STRING LOADING TESTS
// =============================================================================

TEST(ProtoConfigLoaderTest, LoadsValidJsonString) {
    const char* json = R"({
        "schema_version": 1,
        "bootstrap": {
            "server": {"port": 8080},
            "threading": {"worker_threads": 4, "io_service_threads": 2}
        }
    })";
    
    auto result = ProtoConfigLoader::loadFromString(json);
    
    ASSERT_TRUE(result.success) << result.error;
    EXPECT_EQ(result.config.schema_version(), 1);
    EXPECT_EQ(result.config.bootstrap().server().port(), 8080);
}

TEST(ProtoConfigLoaderTest, FailsOnInvalidJson) {
    auto result = ProtoConfigLoader::loadFromString("not valid json");
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}

TEST(ProtoConfigLoaderTest, FailsOnEmptyString) {
    auto result = ProtoConfigLoader::loadFromString("");
    
    EXPECT_FALSE(result.success);
}

TEST(ProtoConfigLoaderTest, AcceptsEmptyObject) {
    auto result = ProtoConfigLoader::loadFromString("{}");
    
    // Empty config is auto-migrated (schema_version 0 → 1)
    // Migration succeeds, but has no data
    ASSERT_TRUE(result.success) << result.error;
    EXPECT_EQ(result.config.schema_version(), 1);  // Migrated to v1
}

TEST(ProtoConfigLoaderTest, IgnoresUnknownFields) {
    const char* json = R"({
        "schema_version": 1,
        "future_field": "ignored",
        "bootstrap": {
            "server": {"port": 8080},
            "threading": {"worker_threads": 4, "io_service_threads": 2}
        }
    })";
    
    auto result = ProtoConfigLoader::loadFromString(json);
    
    ASSERT_TRUE(result.success) << result.error;
    EXPECT_EQ(result.config.bootstrap().server().port(), 8080);
}

// =============================================================================
// VALIDATION TESTS
// =============================================================================

TEST(ProtoConfigLoaderTest, SchemaVersion0IsMigrated) {
    const char* json = R"({
        "schema_version": 0
    })";
    
    auto result = ProtoConfigLoader::loadFromString(json);
    
    // schema_version 0 is treated as legacy and normalized to v1
    ASSERT_TRUE(result.success) << result.error;
    EXPECT_EQ(result.config.schema_version(), 1);  // Normalized
}

TEST(ProtoConfigLoaderTest, ValidatesPortRange) {
    const char* json = R"({
        "schema_version": 1,
        "bootstrap": {"server": {"port": 70000}}
    })";
    
    auto result = ProtoConfigLoader::loadFromString(json);
    
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error.find("port"), std::string::npos);
}

TEST(ProtoConfigLoaderTest, ValidatesPortZero) {
    const char* json = R"({
        "schema_version": 1,
        "bootstrap": {"server": {"port": 0}}
    })";
    
    auto result = ProtoConfigLoader::loadFromString(json);
    
    EXPECT_FALSE(result.success);
}

TEST(ProtoConfigLoaderTest, ValidatesWorkerThreads) {
    const char* json = R"({
        "schema_version": 1,
        "bootstrap": {"threading": {"worker_threads": 0, "io_service_threads": 1}}
    })";
    
    auto result = ProtoConfigLoader::loadFromString(json);
    
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error.find("worker_threads"), std::string::npos);
}

TEST(ProtoConfigLoaderTest, ValidatesTracingSampleRate) {
    const char* json = R"({
        "schema_version": 1,
        "operational": {
            "observability": {"tracing_sample_rate": 1.5}
        }
    })";
    
    auto result = ProtoConfigLoader::loadFromString(json);
    
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error.find("tracing_sample_rate"), std::string::npos);
}

TEST(ProtoConfigLoaderTest, AllowsValidTracingSampleRate) {
    const char* json = R"({
        "schema_version": 1,
        "operational": {
            "observability": {"tracing_sample_rate": 0.1}
        }
    })";
    
    auto result = ProtoConfigLoader::loadFromString(json);
    
    ASSERT_TRUE(result.success) << result.error;
}

// =============================================================================
// FILE LOADING TESTS
// =============================================================================

class FileLoadingTest : public ::testing::Test {
protected:
    std::string test_file_;
    
    void SetUp() override {
        test_file_ = "/tmp/config_loader_test_" + std::to_string(std::rand()) + ".json";
    }
    
    void TearDown() override {
        std::remove(test_file_.c_str());
    }
    
    void writeFile(const std::string& content) {
        std::ofstream file(test_file_);
        file << content;
        file.close();
    }
};

TEST_F(FileLoadingTest, LoadsValidFile) {
    writeFile(R"({
        "schema_version": 1,
        "bootstrap": {
            "server": {"port": 8080},
            "threading": {"worker_threads": 4, "io_service_threads": 2}
        }
    })");
    
    auto result = ProtoConfigLoader::loadFromFile(test_file_);
    
    ASSERT_TRUE(result.success) << result.error;
    EXPECT_EQ(result.config.bootstrap().server().port(), 8080);
}

TEST_F(FileLoadingTest, FailsOnNonExistentFile) {
    auto result = ProtoConfigLoader::loadFromFile("/nonexistent/path/config.json");
    
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error.find("Failed to open"), std::string::npos);
}

TEST_F(FileLoadingTest, FailsOnEmptyFile) {
    writeFile("");
    
    auto result = ProtoConfigLoader::loadFromFile(test_file_);
    
    EXPECT_FALSE(result.success);
}

TEST_F(FileLoadingTest, FailsOnMalformedFile) {
    writeFile("{not valid json");
    
    auto result = ProtoConfigLoader::loadFromFile(test_file_);
    
    EXPECT_FALSE(result.success);
}

// =============================================================================
// MERGE TESTS
// =============================================================================

TEST(MergeTest, MergesRuntimeConfig) {
    Config base;
    base.set_schema_version(1);
    base.mutable_bootstrap()->mutable_server()->set_port(8080);
    base.mutable_runtime()->mutable_rate_limiting()->set_global_rps_limit(100000);
    
    Config overlay;
    overlay.mutable_runtime()->mutable_rate_limiting()->set_global_rps_limit(50000);
    overlay.mutable_runtime()->mutable_feature_flags()->set_enable_caching(true);
    
    ProtoConfigLoader::mergeRuntime(base, overlay);
    
    EXPECT_EQ(base.bootstrap().server().port(), 8080);  // Unchanged
    EXPECT_EQ(base.runtime().rate_limiting().global_rps_limit(), 50000);  // Updated
    EXPECT_TRUE(base.runtime().feature_flags().enable_caching());  // New
}

TEST(MergeTest, MergesOperationalConfig) {
    Config base;
    base.set_schema_version(1);
    base.mutable_operational()->mutable_logging()->set_level("INFO");
    base.mutable_operational()->mutable_timeouts()->set_request_ms(5000);
    
    Config overlay;
    overlay.mutable_operational()->mutable_logging()->set_level("DEBUG");
    
    ProtoConfigLoader::mergeOperational(base, overlay);
    
    EXPECT_EQ(base.operational().logging().level(), "DEBUG");  // Updated
}

TEST(MergeTest, NoMergeIfOverlayMissing) {
    Config base;
    base.set_schema_version(1);
    base.mutable_runtime()->mutable_rate_limiting()->set_global_rps_limit(100000);
    
    Config overlay;  // No runtime set
    
    ProtoConfigLoader::mergeRuntime(base, overlay);
    
    EXPECT_EQ(base.runtime().rate_limiting().global_rps_limit(), 100000);  // Unchanged
}

// =============================================================================
// FULL CONFIG LOAD TESTS
// =============================================================================

TEST(FullConfigTest, LoadsCompleteConfig) {
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
    
    auto result = ProtoConfigLoader::loadFromString(json);
    
    ASSERT_TRUE(result.success) << result.error;
    EXPECT_EQ(result.config.schema_version(), 1);
    EXPECT_EQ(result.config.bootstrap().server().port(), 8080);
    EXPECT_EQ(result.config.bootstrap().service().name(), "uri-shortener");
    EXPECT_EQ(result.config.operational().logging().level(), "INFO");
    EXPECT_EQ(result.config.runtime().rate_limiting().global_rps_limit(), 100000);
    EXPECT_TRUE(result.config.runtime().feature_flags().enable_caching());
}

} // namespace url_shortener::test
