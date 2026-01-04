/// @file proto_config_extended_test.cpp
/// @brief Extended TDD tests for protobuf config - edge cases, validation, file
/// I/O
/// @note Tests for new config structure with imported library definitions

#include "execution.pb.h"
#include "http2client.pb.h"
#include "http2server.pb.h"
#include "observability.pb.h"
#include "resilience.pb.h"
#include "uri_shortener.pb.h"

#include <atomic>
#include <fstream>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>
#include <gtest/gtest.h>
#include <limits>
#include <thread>
#include <vector>

namespace uri_shortener::test {

// Global test environment for Protobuf cleanup
class ProtobufEnvironment : public ::testing::Environment {
public:
  ~ProtobufEnvironment() override = default;
  void TearDown() override {
    google::protobuf::ShutdownProtobufLibrary();
  }
};

[[maybe_unused]] static ::testing::Environment *const protobuf_env =
    ::testing::AddGlobalTestEnvironment(new ProtobufEnvironment);

// =============================================================================
// EDGE CASE - BOUNDARY VALUES
// =============================================================================

TEST(EdgeCasesTest, PortZero) {
  astra::http2::ServerConfig server;
  server.set_port(0);
  EXPECT_EQ(server.port(), 0);
}

TEST(EdgeCasesTest, PortMaxUint32) {
  astra::http2::ServerConfig server;
  server.set_port(std::numeric_limits<uint32_t>::max());
  EXPECT_EQ(server.port(), std::numeric_limits<uint32_t>::max());
}

TEST(EdgeCasesTest, WorkerCountMax) {
  execution::PoolExecutorConfig pe;
  pe.set_num_workers(std::numeric_limits<uint32_t>::max());
  EXPECT_EQ(pe.num_workers(), std::numeric_limits<uint32_t>::max());
}

TEST(EdgeCasesTest, TraceSampleRateZero) {
  observability::Config obs;
  obs.set_trace_sample_rate(0.0);
  EXPECT_DOUBLE_EQ(obs.trace_sample_rate(), 0.0);
}

TEST(EdgeCasesTest, TraceSampleRateOne) {
  observability::Config obs;
  obs.set_trace_sample_rate(1.0);
  EXPECT_DOUBLE_EQ(obs.trace_sample_rate(), 1.0);
}

TEST(EdgeCasesTest, MaxConcurrentRequestsZero) {
  resilience::LoadShedderPolicy ls;
  ls.set_max_concurrent_requests(0);
  EXPECT_EQ(ls.max_concurrent_requests(), 0);
}

// =============================================================================
// EDGE CASE - EMPTY AND SPECIAL STRINGS
// =============================================================================

TEST(StringEdgeCasesTest, EmptyServiceName) {
  uri_shortener::ServiceConfig service;
  service.set_name("");
  EXPECT_EQ(service.name(), "");
}

TEST(StringEdgeCasesTest, VeryLongServiceName) {
  uri_shortener::ServiceConfig service;
  std::string long_name(1000, 'a');
  service.set_name(long_name);
  EXPECT_EQ(service.name(), long_name);
}

TEST(StringEdgeCasesTest, UnicodeServiceName) {
  uri_shortener::ServiceConfig service;
  service.set_name("サービス名");
  EXPECT_EQ(service.name(), "サービス名");
}

TEST(StringEdgeCasesTest, ServerAddressIPv6) {
  astra::http2::ServerConfig server;
  server.set_address("::1");
  EXPECT_EQ(server.address(), "::1");
}

TEST(StringEdgeCasesTest, ServerAddressHostname) {
  astra::http2::ServerConfig server;
  server.set_address("localhost");
  EXPECT_EQ(server.address(), "localhost");
}

TEST(StringEdgeCasesTest, LogLevelTrace) {
  observability::Config obs;
  obs.set_log_level("TRACE");
  EXPECT_EQ(obs.log_level(), "TRACE");
}

TEST(StringEdgeCasesTest, LogLevelError) {
  observability::Config obs;
  obs.set_log_level("ERROR");
  EXPECT_EQ(obs.log_level(), "ERROR");
}

// =============================================================================
// JSON PARSING - EDGE CASES
// =============================================================================

TEST(JsonEdgeCasesTest, ParsesNullValues) {
  const char *json = R"({
        "schema_version": 1,
        "bootstrap": null
    })";

  uri_shortener::Config config;
  google::protobuf::util::JsonParseOptions options;
  options.ignore_unknown_fields = true;
  auto status =
      google::protobuf::util::JsonStringToMessage(json, &config, options);
  EXPECT_TRUE(status.ok());
  EXPECT_FALSE(config.has_bootstrap());
}

TEST(JsonEdgeCasesTest, ParsesEmptyNestedObject) {
  const char *json = R"({
        "bootstrap": {}
    })";

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);
  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(config.has_bootstrap());
  EXPECT_FALSE(config.bootstrap().has_server());
}

TEST(JsonEdgeCasesTest, FailsOnTypeMismatchStringForInt) {
  const char *json = R"({
        "schema_version": "not a number"
    })";

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);
  EXPECT_FALSE(status.ok());
}

TEST(JsonEdgeCasesTest, ParsesMinifiedJson) {
  const char *json =
      R"({"schema_version":1,"bootstrap":{"server":{"port":8080}}})";

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(config.bootstrap().server().port(), 8080);
}

TEST(JsonEdgeCasesTest, FailsOnArrayWhereObjectExpected) {
  const char *json = R"({
        "bootstrap": []
    })";

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);
  EXPECT_FALSE(status.ok());
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

  void WriteFile(const std::string &content) {
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
  WriteFile(
      R"({"schema_version": 1, "bootstrap": {"server": {"port": 9000}}})");

  std::string json = ReadFile();
  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);

  EXPECT_TRUE(status.ok());
  EXPECT_EQ(config.bootstrap().server().port(), 9000);
}

TEST_F(FileIoTest, RoundTripThroughFile) {
  uri_shortener::Config original;
  original.set_schema_version(1);
  original.mutable_bootstrap()->mutable_server()->set_port(8080);
  original.mutable_runtime()
      ->mutable_load_shedder()
      ->set_max_concurrent_requests(10000);

  // Write to file
  std::string json;
  google::protobuf::util::MessageToJsonString(original, &json);
  WriteFile(json);

  // Read back
  std::string read_json = ReadFile();
  uri_shortener::Config parsed;
  google::protobuf::util::JsonStringToMessage(read_json, &parsed);

  EXPECT_TRUE(
      google::protobuf::util::MessageDifferencer::Equals(original, parsed));
}

TEST_F(FileIoTest, HandlesEmptyFile) {
  WriteFile("");
  std::string json = ReadFile();

  uri_shortener::Config config;
  auto status = google::protobuf::util::JsonStringToMessage(json, &config);
  EXPECT_FALSE(status.ok());
}

// =============================================================================
// COPY AND MOVE SEMANTICS
// =============================================================================

TEST(CopyMoveTest, CopyConstructor) {
  uri_shortener::Config original;
  original.set_schema_version(1);
  original.mutable_bootstrap()->mutable_server()->set_port(8080);

  uri_shortener::Config copy(original);

  EXPECT_EQ(copy.schema_version(), 1);
  EXPECT_EQ(copy.bootstrap().server().port(), 8080);
}

TEST(CopyMoveTest, MoveConstructor) {
  uri_shortener::Config original;
  original.set_schema_version(1);
  original.mutable_bootstrap()->mutable_server()->set_port(8080);

  uri_shortener::Config moved(std::move(original));

  EXPECT_EQ(moved.schema_version(), 1);
  EXPECT_EQ(moved.bootstrap().server().port(), 8080);
}

// =============================================================================
// CLEAR AND RESET
// =============================================================================

TEST(ClearResetTest, ClearConfig) {
  uri_shortener::Config config;
  config.set_schema_version(1);
  config.mutable_bootstrap()->mutable_server()->set_port(8080);

  config.Clear();

  EXPECT_EQ(config.schema_version(), 0);
  EXPECT_FALSE(config.has_bootstrap());
}

// =============================================================================
// MESSAGE DIFFERENCER EDGE CASES
// =============================================================================

TEST(DifferencerEdgeCasesTest, BothEmpty) {
  uri_shortener::Config config1, config2;
  EXPECT_TRUE(
      google::protobuf::util::MessageDifferencer::Equals(config1, config2));
}

TEST(DifferencerEdgeCasesTest, OneEmptyOneNot) {
  uri_shortener::Config config1, config2;
  config2.set_schema_version(1);
  EXPECT_FALSE(
      google::protobuf::util::MessageDifferencer::Equals(config1, config2));
}

// =============================================================================
// SERIALIZATION EDGE CASES
// =============================================================================

TEST(SerializationEdgeCasesTest, SerializeEmptyConfig) {
  uri_shortener::Config config;
  std::string binary;
  EXPECT_TRUE(config.SerializeToString(&binary));
  EXPECT_TRUE(binary.empty() || binary.size() < 5); // Empty message is small
}

TEST(SerializationEdgeCasesTest, SerializeLargeConfig) {
  uri_shortener::Config config;
  config.set_schema_version(99999);
  std::string long_name(10000, 'a');
  config.mutable_bootstrap()->mutable_service()->set_name(long_name);

  std::string binary;
  EXPECT_TRUE(config.SerializeToString(&binary));

  uri_shortener::Config parsed;
  EXPECT_TRUE(parsed.ParseFromString(binary));
  EXPECT_EQ(parsed.bootstrap().service().name().size(), 10000);
}

// =============================================================================
// CONCURRENT ACCESS TESTS
// =============================================================================

TEST(ConcurrencyTest, ConcurrentReads) {
  uri_shortener::Config config;
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

  for (auto &t : threads) {
    t.join();
  }
  EXPECT_EQ(read_count, 1000);
}

// =============================================================================
// NESTED MESSAGE TESTS
// =============================================================================

TEST(NestedMessageTest, DeepNesting) {
  uri_shortener::Config config;
  config.mutable_bootstrap()->mutable_observability()->set_otlp_endpoint(
      "http://otel:4317");

  EXPECT_EQ(config.bootstrap().observability().otlp_endpoint(),
            "http://otel:4317");
}

TEST(NestedMessageTest, DataserviceResilience) {
  uri_shortener::Config config;
  config.mutable_bootstrap()
      ->mutable_dataservice()
      ->mutable_resilience()
      ->mutable_retry()
      ->set_max_attempts(3);

  EXPECT_EQ(
      config.bootstrap().dataservice().resilience().retry().max_attempts(), 3);
}

} // namespace uri_shortener::test
