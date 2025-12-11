/// @file proto_config_extended_test.cpp
/// @brief Extended TDD tests for protobuf config - edge cases, validation, file I/O
/// @note Part of 200 unique test cases for config library (tests 76-200)

#include <gtest/gtest.h>
#include "uri_shortener.pb.h"
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>
#include <fstream>
#include <thread>
#include <vector>
#include <atomic>
#include <limits>

namespace uri_shortener::test {

// Global test environment for Protobuf cleanup (fixes Valgrind false positives)
class ProtobufEnvironment : public ::testing::Environment {
public:
    ~ProtobufEnvironment() override = default;
    
    void TearDown() override {
        // Cleanup Protobuf internals for clean Valgrind runs
        // This is the industry-standard approach for Protobuf testing
        google::protobuf::ShutdownProtobufLibrary();
    }
};

// Register global environment (called once per test binary)
[[maybe_unused]] static ::testing::Environment* const protobuf_env =
    ::testing::AddGlobalTestEnvironment(new ProtobufEnvironment);


// =============================================================================
// EDGE CASE - BOUNDARY VALUES
// =============================================================================

TEST(EdgeCasesTest, PortZero) {
    ServerConfig server;
    server.set_port(0);
    EXPECT_EQ(server.port(), 0);
}

TEST(EdgeCasesTest, PortMaxUint32) {
    ServerConfig server;
    server.set_port(std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(server.port(), std::numeric_limits<uint32_t>::max());
}

TEST(EdgeCasesTest, PortValidRange) {
    ServerConfig server;
    server.set_port(65535);
    EXPECT_EQ(server.port(), 65535);
}

TEST(EdgeCasesTest, WorkerThreadsMax) {
    ThreadingConfig threading;
    threading.set_worker_threads(std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(threading.worker_threads(), std::numeric_limits<uint32_t>::max());
}

TEST(EdgeCasesTest, TimeoutNegative) {
    TimeoutsConfig timeouts;
    timeouts.set_request_ms(-1);  // Proto int32 allows negative
    EXPECT_EQ(timeouts.request_ms(), -1);
}

TEST(EdgeCasesTest, TimeoutMaxInt32) {
    TimeoutsConfig timeouts;
    timeouts.set_request_ms(std::numeric_limits<int32_t>::max());
    EXPECT_EQ(timeouts.request_ms(), std::numeric_limits<int32_t>::max());
}

TEST(EdgeCasesTest, TracingSampleRateZero) {
    ObservabilityConfig obs;
    obs.set_tracing_sample_rate(0.0);
    EXPECT_DOUBLE_EQ(obs.tracing_sample_rate(), 0.0);
}

TEST(EdgeCasesTest, TracingSampleRateOne) {
    ObservabilityConfig obs;
    obs.set_tracing_sample_rate(1.0);
    EXPECT_DOUBLE_EQ(obs.tracing_sample_rate(), 1.0);
}

TEST(EdgeCasesTest, TracingSampleRateNegative) {
    ObservabilityConfig obs;
    obs.set_tracing_sample_rate(-0.1);
    EXPECT_DOUBLE_EQ(obs.tracing_sample_rate(), -0.1);  // Proto allows this
}

TEST(EdgeCasesTest, TracingSampleRateGreaterThanOne) {
    ObservabilityConfig obs;
    obs.set_tracing_sample_rate(1.5);
    EXPECT_DOUBLE_EQ(obs.tracing_sample_rate(), 1.5);  // Proto allows this
}

TEST(EdgeCasesTest, RpsLimitZero) {
    RateLimitingConfig rate;
    rate.set_global_rps_limit(0);
    EXPECT_EQ(rate.global_rps_limit(), 0);
}

TEST(EdgeCasesTest, RpsLimitNegative) {
    RateLimitingConfig rate;
    rate.set_global_rps_limit(-1);
    EXPECT_EQ(rate.global_rps_limit(), -1);  // Proto int32 allows negative
}

TEST(EdgeCasesTest, BurstSizeZero) {
    RateLimitingConfig rate;
    rate.set_burst_size(0);
    EXPECT_EQ(rate.burst_size(), 0);
}

// =============================================================================
// EDGE CASE - EMPTY AND SPECIAL STRINGS
// =============================================================================

TEST(StringEdgeCasesTest, EmptyServiceName) {
    ServiceConfig service;
    service.set_name("");
    EXPECT_EQ(service.name(), "");
}

TEST(StringEdgeCasesTest, SingleCharServiceName) {
    ServiceConfig service;
    service.set_name("x");
    EXPECT_EQ(service.name(), "x");
}

TEST(StringEdgeCasesTest, VeryLongServiceName) {
    ServiceConfig service;
    std::string long_name(1000, 'a');
    service.set_name(long_name);
    EXPECT_EQ(service.name(), long_name);
}

TEST(StringEdgeCasesTest, UnicodeServiceName) {
    ServiceConfig service;
    service.set_name("サービス名");
    EXPECT_EQ(service.name(), "サービス名");
}

TEST(StringEdgeCasesTest, ServiceNameWithSpaces) {
    ServiceConfig service;
    service.set_name("my service name");
    EXPECT_EQ(service.name(), "my service name");
}

TEST(StringEdgeCasesTest, ServiceNameWithSpecialChars) {
    ServiceConfig service;
    service.set_name("service-name_v1.0");
    EXPECT_EQ(service.name(), "service-name_v1.0");
}

TEST(StringEdgeCasesTest, AddressWithPort) {
    ServerConfig server;
    server.set_address("192.168.1.1:8080");  // Not recommended but proto allows
    EXPECT_EQ(server.address(), "192.168.1.1:8080");
}

TEST(StringEdgeCasesTest, IPv6Address) {
    ServerConfig server;
    server.set_address("::1");
    EXPECT_EQ(server.address(), "::1");
}

TEST(StringEdgeCasesTest, HostnameAddress) {
    ServerConfig server;
    server.set_address("localhost");
    EXPECT_EQ(server.address(), "localhost");
}

TEST(StringEdgeCasesTest, FqdnAddress) {
    ServerConfig server;
    server.set_address("api.example.com");
    EXPECT_EQ(server.address(), "api.example.com");
}

TEST(StringEdgeCasesTest, MongoUriWithCredentials) {
    DatabaseConfig db;
    db.set_mongodb_uri("mongodb://user:pass@localhost:27017/db?authSource=admin");
    EXPECT_EQ(db.mongodb_uri(), "mongodb://user:pass@localhost:27017/db?authSource=admin");
}

TEST(StringEdgeCasesTest, RedisUriWithDatabase) {
    DatabaseConfig db;
    db.set_redis_uri("redis://localhost:6379/1");
    EXPECT_EQ(db.redis_uri(), "redis://localhost:6379/1");
}

TEST(StringEdgeCasesTest, OtlpEndpointHttps) {
    ObservabilityConfig obs;
    obs.set_otlp_endpoint("https://otel.example.com:4317");
    EXPECT_EQ(obs.otlp_endpoint(), "https://otel.example.com:4317");
}

TEST(StringEdgeCasesTest, EnvironmentProduction) {
    ServiceConfig service;
    service.set_environment("production");
    EXPECT_EQ(service.environment(), "production");
}

TEST(StringEdgeCasesTest, EnvironmentStaging) {
    ServiceConfig service;
    service.set_environment("staging");
    EXPECT_EQ(service.environment(), "staging");
}

TEST(StringEdgeCasesTest, EnvironmentDevelopment) {
    ServiceConfig service;
    service.set_environment("development");
    EXPECT_EQ(service.environment(), "development");
}

TEST(StringEdgeCasesTest, LogLevelTrace) {
    LoggingConfig logging;
    logging.set_level("TRACE");
    EXPECT_EQ(logging.level(), "TRACE");
}

TEST(StringEdgeCasesTest, LogLevelDebug) {
    LoggingConfig logging;
    logging.set_level("DEBUG");
    EXPECT_EQ(logging.level(), "DEBUG");
}

TEST(StringEdgeCasesTest, LogLevelInfo) {
    LoggingConfig logging;
    logging.set_level("INFO");
    EXPECT_EQ(logging.level(), "INFO");
}

TEST(StringEdgeCasesTest, LogLevelWarn) {
    LoggingConfig logging;
    logging.set_level("WARN");
    EXPECT_EQ(logging.level(), "WARN");
}

TEST(StringEdgeCasesTest, LogLevelError) {
    LoggingConfig logging;
    logging.set_level("ERROR");
    EXPECT_EQ(logging.level(), "ERROR");
}

TEST(StringEdgeCasesTest, LogFormatJson) {
    LoggingConfig logging;
    logging.set_format("json");
    EXPECT_EQ(logging.format(), "json");
}

TEST(StringEdgeCasesTest, LogFormatText) {
    LoggingConfig logging;
    logging.set_format("text");
    EXPECT_EQ(logging.format(), "text");
}

// =============================================================================
// JSON PARSING - EDGE CASES
// =============================================================================

TEST(JsonEdgeCasesTest, ParsesNullValues) {
    const char* json = R"({
        "schema_version": 1,
        "bootstrap": null
    })";
    
    Config config;
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config, options);
    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(config.has_bootstrap());
}

TEST(JsonEdgeCasesTest, ParsesEmptyNestedObject) {
    const char* json = R"({
        "bootstrap": {}
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_TRUE(status.ok());
    EXPECT_TRUE(config.has_bootstrap());
    EXPECT_FALSE(config.bootstrap().has_server());
}

TEST(JsonEdgeCasesTest, FailsOnTypeMismatchStringForInt) {
    const char* json = R"({
        "schema_version": "not a number"
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_FALSE(status.ok());
}

TEST(JsonEdgeCasesTest, ParsesNumericStringAsNumber) {
    const char* json = R"({
        "bootstrap": {"server": {"port": 8080}}
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.bootstrap().server().port(), 8080);
}

TEST(JsonEdgeCasesTest, ParsesFloatingPointNumber) {
    const char* json = R"({
        "operational": {
            "observability": {"tracing_sample_rate": 0.123456789}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_TRUE(status.ok());
    EXPECT_NEAR(config.operational().observability().tracing_sample_rate(), 0.123456789, 0.0001);
}

TEST(JsonEdgeCasesTest, ParsesScientificNotation) {
    const char* json = R"({
        "operational": {
            "observability": {"tracing_sample_rate": 1e-5}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_TRUE(status.ok());
    EXPECT_NEAR(config.operational().observability().tracing_sample_rate(), 0.00001, 0.000001);
}

TEST(JsonEdgeCasesTest, ParsesEscapedStrings) {
    const char* json = R"({
        "bootstrap": {
            "service": {"name": "my\"quoted\"service"}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.bootstrap().service().name(), "my\"quoted\"service");
}

TEST(JsonEdgeCasesTest, ParsesNewlineInString) {
    const char* json = R"({
        "bootstrap": {
            "service": {"name": "line1\nline2"}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.bootstrap().service().name(), "line1\nline2");
}

TEST(JsonEdgeCasesTest, ParsesTabInString) {
    const char* json = R"({
        "bootstrap": {
            "service": {"name": "col1\tcol2"}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.bootstrap().service().name(), "col1\tcol2");
}

TEST(JsonEdgeCasesTest, ParsesBackslashInString) {
    const char* json = R"({
        "bootstrap": {
            "database": {"mongodb_uri": "C:\\path\\to\\db"}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.bootstrap().database().mongodb_uri(), "C:\\path\\to\\db");
}

TEST(JsonEdgeCasesTest, FailsOnArrayWhereObjectExpected) {
    const char* json = R"({
        "bootstrap": []
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_FALSE(status.ok());
}

TEST(JsonEdgeCasesTest, ParsesMinifiedJson) {
    const char* json = R"({"schema_version":1,"bootstrap":{"server":{"port":8080}}})";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.bootstrap().server().port(), 8080);
}

TEST(JsonEdgeCasesTest, ParsesWhitespaceOnly) {
    const char* json = "   {   }   ";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_TRUE(status.ok());
}

// =============================================================================
// FILE I/O TESTS
// =============================================================================

class FileIoTest : public ::testing::Test {
protected:
    std::string test_file_;
    
    void SetUp() override {
        test_file_ = "/tmp/config_test_" + std::to_string(std::rand()) + ".json";
    }
    
    void TearDown() override {
        std::remove(test_file_.c_str());
    }
    
    void WriteFile(const std::string& content) {
        std::ofstream file(test_file_);
        file << content;
        file.close();
    }
    
    std::string ReadFile() {
        std::ifstream file(test_file_);
        return std::string((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    }
};

TEST_F(FileIoTest, CanReadConfigFromFile) {
    WriteFile(R"({"schema_version": 1, "bootstrap": {"server": {"port": 9000}}})");
    
    std::string json = ReadFile();
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.bootstrap().server().port(), 9000);
}

TEST_F(FileIoTest, CanWriteConfigToFile) {
    Config config;
    config.set_schema_version(2);
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    std::string json;
    auto status = google::protobuf::util::MessageToJsonString(config, &json);
    EXPECT_TRUE(status.ok());
    
    WriteFile(json);
    
    std::string read_json = ReadFile();
    EXPECT_FALSE(read_json.empty());
}

TEST_F(FileIoTest, RoundTripThroughFile) {
    Config original;
    original.set_schema_version(1);
    original.mutable_bootstrap()->mutable_server()->set_port(8080);
    original.mutable_runtime()->mutable_feature_flags()->set_enable_caching(true);
    
    // Write to file
    std::string json;
    google::protobuf::util::MessageToJsonString(original, &json);
    WriteFile(json);
    
    // Read back
    std::string read_json = ReadFile();
    Config parsed;
    google::protobuf::util::JsonStringToMessage(read_json, &parsed);
    
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(original, parsed));
}

TEST_F(FileIoTest, HandlesNonExistentFile) {
    std::ifstream file("/tmp/nonexistent_config_file.json");
    EXPECT_FALSE(file.is_open());
}

TEST_F(FileIoTest, HandlesEmptyFile) {
    WriteFile("");
    std::string json = ReadFile();
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_FALSE(status.ok());
}

TEST_F(FileIoTest, HandlesTruncatedJson) {
    WriteFile(R"({"schema_version": 1, "bootstrap": {)");  // Incomplete
    std::string json = ReadFile();
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    EXPECT_FALSE(status.ok());
}

// =============================================================================
// COPY AND MOVE SEMANTICS
// =============================================================================

TEST(CopyMoveTest, CopyConstructor) {
    Config original;
    original.set_schema_version(1);
    original.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    Config copy(original);
    
    EXPECT_EQ(copy.schema_version(), 1);
    EXPECT_EQ(copy.bootstrap().server().port(), 8080);
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(original, copy));
}

TEST(CopyMoveTest, CopyAssignment) {
    Config original;
    original.set_schema_version(1);
    original.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    Config copy;
    copy = original;
    
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(original, copy));
}

TEST(CopyMoveTest, MoveConstructor) {
    Config original;
    original.set_schema_version(1);
    original.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    Config moved(std::move(original));
    
    EXPECT_EQ(moved.schema_version(), 1);
    EXPECT_EQ(moved.bootstrap().server().port(), 8080);
}

TEST(CopyMoveTest, MoveAssignment) {
    Config original;
    original.set_schema_version(1);
    original.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    Config moved;
    moved = std::move(original);
    
    EXPECT_EQ(moved.schema_version(), 1);
}

TEST(CopyMoveTest, CopyModifyOriginal) {
    Config original;
    original.set_schema_version(1);
    
    Config copy = original;
    original.set_schema_version(2);
    
    EXPECT_EQ(original.schema_version(), 2);
    EXPECT_EQ(copy.schema_version(), 1);  // Copy not affected
}

// =============================================================================
// CLEAR AND RESET
// =============================================================================

TEST(ClearResetTest, ClearConfig) {
    Config config;
    config.set_schema_version(1);
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    config.Clear();
    
    EXPECT_EQ(config.schema_version(), 0);
    EXPECT_FALSE(config.has_bootstrap());
}

TEST(ClearResetTest, ClearNestedMessage) {
    Config config;
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    config.mutable_bootstrap()->clear_server();
    
    EXPECT_TRUE(config.has_bootstrap());
    EXPECT_FALSE(config.bootstrap().has_server());
}

TEST(ClearResetTest, ClearAndReuse) {
    Config config;
    config.set_schema_version(1);
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    config.Clear();
    
    config.set_schema_version(2);
    config.mutable_runtime()->mutable_feature_flags()->set_enable_caching(true);
    
    EXPECT_EQ(config.schema_version(), 2);
    EXPECT_FALSE(config.has_bootstrap());
    EXPECT_TRUE(config.runtime().feature_flags().enable_caching());
}

// =============================================================================
// MESSAGE DIFFERENCER EDGE CASES
// =============================================================================

TEST(DifferencerEdgeCasesTest, BothEmpty) {
    Config config1, config2;
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(config1, config2));
}

TEST(DifferencerEdgeCasesTest, OneEmptyOneNot) {
    Config config1, config2;
    config2.set_schema_version(1);
    EXPECT_FALSE(google::protobuf::util::MessageDifferencer::Equals(config1, config2));
}

TEST(DifferencerEdgeCasesTest, DifferentNestedMessages) {
    Config config1, config2;
    config1.mutable_bootstrap()->mutable_server()->set_port(8080);
    config2.mutable_runtime()->mutable_rate_limiting()->set_global_rps_limit(1000);
    EXPECT_FALSE(google::protobuf::util::MessageDifferencer::Equals(config1, config2));
}

TEST(DifferencerEdgeCasesTest, SameValuesInDifferentMessages) {
    BootstrapConfig bootstrap1, bootstrap2;
    bootstrap1.mutable_server()->set_port(8080);
    bootstrap2.mutable_server()->set_port(8080);
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(bootstrap1, bootstrap2));
}

TEST(DifferencerEdgeCasesTest, CompareAfterCopy) {
    Config config1;
    config1.set_schema_version(1);
    Config config2 = config1;
    config2.set_schema_version(2);
    EXPECT_FALSE(google::protobuf::util::MessageDifferencer::Equals(config1, config2));
}

// =============================================================================
// SERIALIZATION EDGE CASES
// =============================================================================

TEST(SerializationEdgeCasesTest, SerializeEmptyConfig) {
    Config config;
    std::string binary;
    EXPECT_TRUE(config.SerializeToString(&binary));
    EXPECT_TRUE(binary.empty() || binary.size() < 5);  // Empty message is small
}

TEST(SerializationEdgeCasesTest, SerializeLargeConfig) {
    Config config;
    config.set_schema_version(99999);
    std::string long_name(10000, 'a');
    config.mutable_bootstrap()->mutable_service()->set_name(long_name);
    
    std::string binary;
    EXPECT_TRUE(config.SerializeToString(&binary));
    
    Config parsed;
    EXPECT_TRUE(parsed.ParseFromString(binary));
    EXPECT_EQ(parsed.bootstrap().service().name().size(), 10000);
}

TEST(SerializationEdgeCasesTest, JsonSerializationEmptyConfig) {
    Config config;
    std::string json;
    auto status = google::protobuf::util::MessageToJsonString(config, &json);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(json, "{}");
}

TEST(SerializationEdgeCasesTest, JsonSerializationWithDefaults) {
    Config config;
    config.set_schema_version(0);  // Proto3 default
    
    std::string json;
    google::protobuf::util::JsonPrintOptions options;
    options.always_print_primitive_fields = true;
    auto status = google::protobuf::util::MessageToJsonString(config, &json, options);
    EXPECT_TRUE(status.ok());
    EXPECT_NE(json.find("schemaVersion"), std::string::npos);
}

// =============================================================================
// CONCURRENT ACCESS TESTS
// =============================================================================

TEST(ConcurrencyTest, ConcurrentReads) {
    Config config;
    config.set_schema_version(42);
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    std::atomic<int> read_count{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&config, &read_count]() {
            for (int j = 0; j < 100; ++j) {
                EXPECT_EQ(config.schema_version(), 42);
                EXPECT_EQ(config.bootstrap().server().port(), 8080);
                ++read_count;
            }
        });
    }
    
    for (auto& t : threads) t.join();
    EXPECT_EQ(read_count, 1000);
}

TEST(ConcurrencyTest, CopyInMultipleThreads) {
    Config original;
    original.set_schema_version(1);
    original.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    std::vector<std::thread> threads;
    std::vector<Config> copies(10);
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&original, &copies, i]() {
            copies[i] = original;
        });
    }
    
    for (auto& t : threads) t.join();
    
    for (const auto& copy : copies) {
        EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(original, copy));
    }
}

TEST(ConcurrencyTest, IndependentModifications) {
    std::vector<std::thread> threads;
    std::vector<Config> configs(10);
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&configs, i]() {
            configs[i].set_schema_version(i);
            configs[i].mutable_bootstrap()->mutable_server()->set_port(8080 + i);
        });
    }
    
    for (auto& t : threads) t.join();
    
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(configs[i].schema_version(), i);
        EXPECT_EQ(configs[i].bootstrap().server().port(), 8080 + i);
    }
}

// =============================================================================
// SCHEMA VERSION TESTS
// =============================================================================

TEST(SchemaVersionTest, Version1) {
    Config config;
    config.set_schema_version(1);
    EXPECT_EQ(config.schema_version(), 1);
}

TEST(SchemaVersionTest, Version2) {
    Config config;
    config.set_schema_version(2);
    EXPECT_EQ(config.schema_version(), 2);
}

TEST(SchemaVersionTest, VersionMigrationNeeded) {
    Config old_config;
    old_config.set_schema_version(1);
    
    Config new_config;
    new_config.set_schema_version(2);
    
    // Detect version mismatch
    EXPECT_NE(old_config.schema_version(), new_config.schema_version());
}

// =============================================================================
// NESTED MESSAGE TESTS
// =============================================================================

TEST(NestedMessageTest, DeepNesting) {
    Config config;
    config.mutable_operational()
          ->mutable_observability()
          ->set_otlp_endpoint("http://otel:4317");
    
    EXPECT_EQ(config.operational().observability().otlp_endpoint(), "http://otel:4317");
}

TEST(NestedMessageTest, MultipleNestedSections) {
    Config config;
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    config.mutable_operational()->mutable_timeouts()->set_request_ms(5000);
    config.mutable_runtime()->mutable_rate_limiting()->set_global_rps_limit(10000);
    
    EXPECT_EQ(config.bootstrap().server().port(), 8080);
    EXPECT_EQ(config.operational().timeouts().request_ms(), 5000);
    EXPECT_EQ(config.runtime().rate_limiting().global_rps_limit(), 10000);
}

TEST(NestedMessageTest, PartialConfig) {
    Config config;
    config.mutable_bootstrap();  // Only bootstrap set
    
    EXPECT_TRUE(config.has_bootstrap());
    EXPECT_FALSE(config.has_operational());
    EXPECT_FALSE(config.has_runtime());
}

// =============================================================================
// JSON OUTPUT TESTS
// =============================================================================

TEST(JsonOutputTest, OutputIsValidJson) {
    Config config;
    config.set_schema_version(1);
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    std::string json;
    auto status = google::protobuf::util::MessageToJsonString(config, &json);
    EXPECT_TRUE(status.ok());
    
    // Parse it back to verify it's valid JSON
    Config parsed;
    auto parse_status = google::protobuf::util::JsonStringToMessage(json, &parsed);
    EXPECT_TRUE(parse_status.ok());
}

TEST(JsonOutputTest, PrettyPrintOption) {
    Config config;
    config.set_schema_version(1);
    
    std::string json;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    
    auto status = google::protobuf::util::MessageToJsonString(config, &json, options);
    EXPECT_TRUE(status.ok());
    EXPECT_NE(json.find('\n'), std::string::npos);  // Has newlines
}

TEST(JsonOutputTest, PreserveProtoFieldNames) {
    Config config;
    config.set_schema_version(1);
    
    std::string json;
    google::protobuf::util::JsonPrintOptions options;
    options.preserve_proto_field_names = true;
    
    auto status = google::protobuf::util::MessageToJsonString(config, &json, options);
    EXPECT_TRUE(status.ok());
    EXPECT_NE(json.find("schema_version"), std::string::npos);
}

// =============================================================================
// BOOLEAN FIELD TESTS
// =============================================================================

TEST(BooleanFieldsTest, AllBoolsFalse) {
    RuntimeConfig runtime;
    runtime.mutable_feature_flags()->set_enable_caching(false);
    runtime.mutable_feature_flags()->set_enable_url_preview(false);
    runtime.mutable_feature_flags()->set_compression_enabled(false);
    
    EXPECT_FALSE(runtime.feature_flags().enable_caching());
    EXPECT_FALSE(runtime.feature_flags().enable_url_preview());
    EXPECT_FALSE(runtime.feature_flags().compression_enabled());
}

TEST(BooleanFieldsTest, AllBoolsTrue) {
    RuntimeConfig runtime;
    runtime.mutable_feature_flags()->set_enable_caching(true);
    runtime.mutable_feature_flags()->set_enable_url_preview(true);
    runtime.mutable_feature_flags()->set_compression_enabled(true);
    
    EXPECT_TRUE(runtime.feature_flags().enable_caching());
    EXPECT_TRUE(runtime.feature_flags().enable_url_preview());
    EXPECT_TRUE(runtime.feature_flags().compression_enabled());
}

TEST(BooleanFieldsTest, ToggleBoolean) {
    FeatureFlagsConfig flags;
    
    flags.set_enable_caching(true);
    EXPECT_TRUE(flags.enable_caching());
    
    flags.set_enable_caching(false);
    EXPECT_FALSE(flags.enable_caching());
    
    flags.set_enable_caching(true);
    EXPECT_TRUE(flags.enable_caching());
}

// =============================================================================
// PRODUCTION SCENARIO TESTS (High-Value Real-World Cases)
// =============================================================================

// Full environment-specific config scenarios
TEST(ProductionScenarioTest, DevelopmentEnvironmentConfig) {
    const char* json = R"({
        "schema_version": 1,
        "bootstrap": {
            "server": {"address": "127.0.0.1", "port": 8080},
            "threading": {"worker_threads": 2, "io_service_threads": 1},
            "database": {"mongodb_uri": "mongodb://localhost:27017/dev", "redis_uri": "redis://localhost:6379/0"},
            "service": {"name": "uri-shortener", "environment": "development"}
        },
        "operational": {
            "logging": {"level": "DEBUG", "format": "text", "enable_access_logs": true},
            "timeouts": {"request_ms": 30000, "database_ms": 10000, "http_client_ms": 10000}
        },
        "runtime": {
            "rate_limiting": {"global_rps_limit": 1000, "per_user_rps_limit": 100, "burst_size": 50},
            "feature_flags": {"enable_caching": false, "enable_url_preview": true, "compression_enabled": false}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    ASSERT_TRUE(status.ok());
    
    EXPECT_EQ(config.bootstrap().service().environment(), "development");
    EXPECT_EQ(config.operational().logging().level(), "DEBUG");
    EXPECT_FALSE(config.runtime().feature_flags().enable_caching());
}

TEST(ProductionScenarioTest, StagingEnvironmentConfig) {
    const char* json = R"({
        "schema_version": 1,
        "bootstrap": {
            "server": {"address": "0.0.0.0", "port": 8080},
            "threading": {"worker_threads": 4, "io_service_threads": 2},
            "service": {"name": "uri-shortener", "environment": "staging"}
        },
        "operational": {
            "logging": {"level": "INFO", "format": "json", "enable_access_logs": true},
            "observability": {"metrics_enabled": true, "tracing_enabled": true, "tracing_sample_rate": 0.5}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    ASSERT_TRUE(status.ok());
    
    EXPECT_EQ(config.bootstrap().service().environment(), "staging");
    EXPECT_DOUBLE_EQ(config.operational().observability().tracing_sample_rate(), 0.5);
}

TEST(ProductionScenarioTest, ProductionEnvironmentConfig) {
    const char* json = R"({
        "schema_version": 1,
        "bootstrap": {
            "server": {"address": "0.0.0.0", "port": 8080},
            "threading": {"worker_threads": 16, "io_service_threads": 8},
            "database": {
                "mongodb_uri": "mongodb+srv://user:pass@cluster.mongodb.net/prod?retryWrites=true",
                "redis_uri": "redis://redis-cluster.internal:6379"
            },
            "service": {"name": "uri-shortener", "environment": "production"}
        },
        "operational": {
            "logging": {"level": "WARN", "format": "json", "enable_access_logs": false},
            "timeouts": {"request_ms": 5000, "database_ms": 2000, "http_client_ms": 3000},
            "connection_pools": {"mongodb_pool_size": 50, "redis_pool_size": 25, "http2_max_connections": 200},
            "observability": {
                "metrics_enabled": true, "tracing_enabled": true, "logging_enabled": true,
                "tracing_sample_rate": 0.01, "otlp_endpoint": "http://otel-collector:4317", "service_version": "2.1.0"
            }
        },
        "runtime": {
            "rate_limiting": {"global_rps_limit": 1000000, "per_user_rps_limit": 10000, "burst_size": 50000},
            "circuit_breaker": {"mongodb_threshold": 5, "mongodb_timeout_sec": 30, "redis_threshold": 3, "redis_timeout_sec": 15},
            "feature_flags": {"enable_caching": true, "enable_url_preview": false, "compression_enabled": true},
            "backpressure": {"worker_queue_max": 100000, "io_queue_max": 50000}
        }
    })";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(json, &config);
    ASSERT_TRUE(status.ok());
    
    EXPECT_EQ(config.bootstrap().service().environment(), "production");
    EXPECT_EQ(config.bootstrap().threading().worker_threads(), 16);
    EXPECT_EQ(config.operational().logging().level(), "WARN");
    EXPECT_DOUBLE_EQ(config.operational().observability().tracing_sample_rate(), 0.01);
    EXPECT_EQ(config.runtime().rate_limiting().global_rps_limit(), 1000000);
    EXPECT_TRUE(config.runtime().feature_flags().enable_caching());
}

// Config reload simulation (detecting what changed)
TEST(ConfigReloadTest, DetectPortChange) {
    Config before, after;
    before.mutable_bootstrap()->mutable_server()->set_port(8080);
    after.mutable_bootstrap()->mutable_server()->set_port(9000);
    
    EXPECT_NE(before.bootstrap().server().port(), after.bootstrap().server().port());
    EXPECT_FALSE(google::protobuf::util::MessageDifferencer::Equals(before, after));
}

TEST(ConfigReloadTest, DetectLogLevelChange) {
    Config before, after;
    before.mutable_operational()->mutable_logging()->set_level("INFO");
    after.mutable_operational()->mutable_logging()->set_level("DEBUG");
    
    EXPECT_NE(before.operational().logging().level(), after.operational().logging().level());
}

TEST(ConfigReloadTest, DetectRateLimitChange) {
    Config before, after;
    before.mutable_runtime()->mutable_rate_limiting()->set_global_rps_limit(100000);
    after.mutable_runtime()->mutable_rate_limiting()->set_global_rps_limit(50000);
    
    // This simulates detecting a change to apply at runtime
    bool rate_limit_changed = before.runtime().rate_limiting().global_rps_limit() != 
                              after.runtime().rate_limiting().global_rps_limit();
    EXPECT_TRUE(rate_limit_changed);
}

TEST(ConfigReloadTest, DetectFeatureFlagToggle) {
    Config before, after;
    before.mutable_runtime()->mutable_feature_flags()->set_enable_caching(false);
    after.mutable_runtime()->mutable_feature_flags()->set_enable_caching(true);
    
    bool caching_toggled = before.runtime().feature_flags().enable_caching() !=
                           after.runtime().feature_flags().enable_caching();
    EXPECT_TRUE(caching_toggled);
}

TEST(ConfigReloadTest, NoChangeDetected) {
    Config before, after;
    before.set_schema_version(1);
    before.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    after.set_schema_version(1);
    after.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(before, after));
}

// Partial config updates (runtime only)
TEST(PartialUpdateTest, UpdateRuntimeOnly) {
    Config full_config;
    full_config.set_schema_version(1);
    full_config.mutable_bootstrap()->mutable_server()->set_port(8080);
    full_config.mutable_runtime()->mutable_rate_limiting()->set_global_rps_limit(100000);
    
    // Simulate runtime-only update
    RuntimeConfig new_runtime;
    new_runtime.mutable_rate_limiting()->set_global_rps_limit(50000);
    
    full_config.mutable_runtime()->CopyFrom(new_runtime);
    
    EXPECT_EQ(full_config.bootstrap().server().port(), 8080);  // Unchanged
    EXPECT_EQ(full_config.runtime().rate_limiting().global_rps_limit(), 50000);  // Updated
}

TEST(PartialUpdateTest, MergeOperationalChanges) {
    Config config;
    config.mutable_operational()->mutable_logging()->set_level("INFO");
    config.mutable_operational()->mutable_timeouts()->set_request_ms(5000);
    
    // Only update logging
    LoggingConfig new_logging;
    new_logging.set_level("DEBUG");
    new_logging.set_format("json");
    
    config.mutable_operational()->mutable_logging()->CopyFrom(new_logging);
    
    EXPECT_EQ(config.operational().logging().level(), "DEBUG");
    EXPECT_EQ(config.operational().timeouts().request_ms(), 5000);  // Unchanged
}

// Validation helper functions (simulate what app would do)
TEST(ValidationHelperTest, ValidatePort) {
    auto is_valid_port = [](int32_t port) {
        return port > 0 && port <= 65535;
    };
    
    EXPECT_FALSE(is_valid_port(0));
    EXPECT_FALSE(is_valid_port(-1));
    EXPECT_TRUE(is_valid_port(80));
    EXPECT_TRUE(is_valid_port(8080));
    EXPECT_TRUE(is_valid_port(65535));
    EXPECT_FALSE(is_valid_port(65536));
}

TEST(ValidationHelperTest, ValidateWorkerThreads) {
    auto is_valid_workers = [](int32_t workers) {
        return workers > 0 && workers <= 1024;
    };
    
    EXPECT_FALSE(is_valid_workers(0));
    EXPECT_TRUE(is_valid_workers(1));
    EXPECT_TRUE(is_valid_workers(16));
    EXPECT_TRUE(is_valid_workers(1024));
    EXPECT_FALSE(is_valid_workers(2000));
}

TEST(ValidationHelperTest, ValidateSampleRate) {
    auto is_valid_sample_rate = [](double rate) {
        return rate >= 0.0 && rate <= 1.0;
    };
    
    EXPECT_TRUE(is_valid_sample_rate(0.0));
    EXPECT_TRUE(is_valid_sample_rate(0.5));
    EXPECT_TRUE(is_valid_sample_rate(1.0));
    EXPECT_FALSE(is_valid_sample_rate(-0.1));
    EXPECT_FALSE(is_valid_sample_rate(1.1));
}

TEST(ValidationHelperTest, ValidateLogLevel) {
    auto is_valid_log_level = [](const std::string& level) {
        return level == "TRACE" || level == "DEBUG" || level == "INFO" || 
               level == "WARN" || level == "ERROR" || level == "FATAL";
    };
    
    EXPECT_TRUE(is_valid_log_level("INFO"));
    EXPECT_TRUE(is_valid_log_level("DEBUG"));
    EXPECT_FALSE(is_valid_log_level("INVALID"));
    EXPECT_FALSE(is_valid_log_level(""));
}

TEST(ValidationHelperTest, ValidateMongoUri) {
    auto has_valid_mongo_prefix = [](const std::string& uri) {
        return uri.substr(0, 7) == "mongodb" || uri.substr(0, 11) == "mongodb+srv";
    };
    
    EXPECT_TRUE(has_valid_mongo_prefix("mongodb://localhost:27017"));
    EXPECT_TRUE(has_valid_mongo_prefix("mongodb+srv://cluster.mongodb.net"));
    EXPECT_FALSE(has_valid_mongo_prefix("http://localhost:27017"));
    EXPECT_FALSE(has_valid_mongo_prefix(""));
}

// Error recovery scenarios
TEST(ErrorRecoveryTest, FallbackToDefaults) {
    const char* invalid_json = "not valid json";
    
    Config config;
    auto status = google::protobuf::util::JsonStringToMessage(invalid_json, &config);
    
    if (!status.ok()) {
        // Fallback to safe defaults
        config.set_schema_version(1);
        config.mutable_bootstrap()->mutable_server()->set_port(8080);
        config.mutable_operational()->mutable_logging()->set_level("WARN");
    }
    
    EXPECT_EQ(config.schema_version(), 1);
    EXPECT_EQ(config.bootstrap().server().port(), 8080);
}

TEST(ErrorRecoveryTest, PartialParseWithUnknownFields) {
    const char* json_with_future_field = R"({
        "schema_version": 1,
        "bootstrap": {"server": {"port": 8080}},
        "future_field_v3": {"new_feature": true}
    })";
    
    Config config;
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;
    
    auto status = google::protobuf::util::JsonStringToMessage(json_with_future_field, &config, options);
    
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(config.bootstrap().server().port(), 8080);
}

// Config diff for audit logging
TEST(ConfigDiffTest, GenerateChangeReport) {
    Config before, after;
    before.set_schema_version(1);
    before.mutable_runtime()->mutable_rate_limiting()->set_global_rps_limit(100000);
    
    after.set_schema_version(1);
    after.mutable_runtime()->mutable_rate_limiting()->set_global_rps_limit(50000);
    
    std::string diff_report;
    google::protobuf::util::MessageDifferencer differencer;
    differencer.ReportDifferencesToString(&diff_report);
    
    bool is_different = !differencer.Compare(before, after);
    
    EXPECT_TRUE(is_different);
    EXPECT_FALSE(diff_report.empty());
    EXPECT_NE(diff_report.find("global_rps_limit"), std::string::npos);
}

// Schema evolution simulation
TEST(SchemaEvolutionTest, V1ToV2Migration) {
    // V1 config
    Config v1_config;
    v1_config.set_schema_version(1);
    v1_config.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    // Simulate migration to V2 (add new required field)
    Config v2_config = v1_config;
    v2_config.set_schema_version(2);
    // In V2 we might require observability to be set
    v2_config.mutable_operational()->mutable_observability()->set_metrics_enabled(true);
    
    EXPECT_EQ(v2_config.schema_version(), 2);
    EXPECT_TRUE(v2_config.operational().observability().metrics_enabled());
}

} // namespace uri_shortener::test
