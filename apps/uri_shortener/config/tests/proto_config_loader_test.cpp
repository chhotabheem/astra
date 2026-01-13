/// @file proto_config_loader_test.cpp
/// @brief TDD tests for ProtoConfigLoader

#include "ProtoConfigLoader.h"

#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>

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
// STRING LOADING TESTS
// =============================================================================

TEST(ProtoConfigLoaderTest, LoadsValidJsonString) {
  const char *json = R"({
        "schema_version": 1,
        "bootstrap": {
            "server": {"uri": "0.0.0.0:8080", "thread_count": 2},
            "execution": {"pool_executor": {"num_workers": 4}}
        }
    })";

  auto result = ProtoConfigLoader::loadFromString(json);

  ASSERT_TRUE(result.is_ok()) << result.error();
  EXPECT_EQ(result.value().schema_version(), 1);
  EXPECT_EQ(result.value().bootstrap().server().uri(), "0.0.0.0:8080");
}

TEST(ProtoConfigLoaderTest, FailsOnInvalidJson) {
  auto result = ProtoConfigLoader::loadFromString("not valid json");

  EXPECT_TRUE(result.is_err());
  EXPECT_FALSE(result.error().empty());
}

TEST(ProtoConfigLoaderTest, FailsOnEmptyString) {
  auto result = ProtoConfigLoader::loadFromString("");

  EXPECT_TRUE(result.is_err());
}

TEST(ProtoConfigLoaderTest, AcceptsEmptyObject) {
  auto result = ProtoConfigLoader::loadFromString("{}");

  ASSERT_TRUE(result.is_ok()) << result.error();
}

TEST(ProtoConfigLoaderTest, IgnoresUnknownFields) {
  const char *json = R"({
        "schema_version": 1,
        "future_field": "ignored",
        "bootstrap": {
            "server": {"uri": "0.0.0.0:8080", "thread_count": 2}
        }
    })";

  auto result = ProtoConfigLoader::loadFromString(json);

  ASSERT_TRUE(result.is_ok()) << result.error();
  EXPECT_EQ(result.value().bootstrap().server().uri(), "0.0.0.0:8080");
}

// =============================================================================
// VALIDATION TESTS
// =============================================================================

TEST(ProtoConfigLoaderTest, ValidatesEmptyUri) {
  const char *json = R"({
        "schema_version": 1,
        "bootstrap": {"server": {"uri": ""}}
    })";

  auto result = ProtoConfigLoader::loadFromString(json);

  EXPECT_TRUE(result.is_err());
  EXPECT_NE(result.error().find("uri"), std::string::npos);
}

TEST(ProtoConfigLoaderTest, ValidatesPoolExecutorWorkers) {
  const char *json = R"({
        "schema_version": 1,
        "bootstrap": {"execution": {"pool_executor": {"num_workers": 0}}}
    })";

  auto result = ProtoConfigLoader::loadFromString(json);

  EXPECT_TRUE(result.is_err());
  EXPECT_NE(result.error().find("num_workers"), std::string::npos);
}

TEST(ProtoConfigLoaderTest, ValidatesTraceSampleRate) {
  const char *json = R"({
        "schema_version": 1,
        "bootstrap": {
            "observability": {"trace_sample_rate": 1.5}
        }
    })";

  auto result = ProtoConfigLoader::loadFromString(json);

  EXPECT_TRUE(result.is_err());
  EXPECT_NE(result.error().find("trace_sample_rate"), std::string::npos);
}

TEST(ProtoConfigLoaderTest, AllowsValidTraceSampleRate) {
  const char *json = R"({
        "schema_version": 1,
        "bootstrap": {
            "observability": {"trace_sample_rate": 0.1}
        }
    })";

  auto result = ProtoConfigLoader::loadFromString(json);

  ASSERT_TRUE(result.is_ok()) << result.error();
}

// =============================================================================
// FILE LOADING TESTS
// =============================================================================

class FileLoadingTest : public ::testing::Test {
protected:
  std::string test_file_;

  void SetUp() override {
    test_file_ =
        "/tmp/config_loader_test_" + std::to_string(std::rand()) + ".json";
  }

  void TearDown() override {
    std::remove(test_file_.c_str());
  }

  void writeFile(const std::string &content) {
    std::ofstream file(test_file_);
    file << content;
    file.close();
  }
};

TEST_F(FileLoadingTest, LoadsValidFile) {
  writeFile(R"({
        "schema_version": 1,
        "bootstrap": {
            "server": {"uri": "0.0.0.0:8080", "thread_count": 2},
            "execution": {"pool_executor": {"num_workers": 4}}
        }
    })");

  auto result = ProtoConfigLoader::loadFromFile(test_file_);

  ASSERT_TRUE(result.is_ok()) << result.error();
  EXPECT_EQ(result.value().bootstrap().server().uri(), "0.0.0.0:8080");
}

TEST_F(FileLoadingTest, FailsOnNonExistentFile) {
  auto result =
      ProtoConfigLoader::loadFromFile("/nonexistent/path/config.json");

  EXPECT_TRUE(result.is_err());
  EXPECT_NE(result.error().find("Failed to open"), std::string::npos);
}

TEST_F(FileLoadingTest, FailsOnEmptyFile) {
  writeFile("");

  auto result = ProtoConfigLoader::loadFromFile(test_file_);

  EXPECT_TRUE(result.is_err());
}

TEST_F(FileLoadingTest, FailsOnMalformedFile) {
  writeFile("{not valid json");

  auto result = ProtoConfigLoader::loadFromFile(test_file_);

  EXPECT_TRUE(result.is_err());
}

// =============================================================================
// FULL CONFIG LOAD TEST
// =============================================================================

TEST(FullConfigTest, LoadsCompleteConfig) {
  const char *json = R"({
        "schema_version": 1,
        "bootstrap": {
            "server": {"uri": "0.0.0.0:8080", "thread_count": 2},
            "execution": {"pool_executor": {"num_workers": 4}},
            "observability": {"service_name": "uri-shortener", "trace_sample_rate": 0.1},
            "dataservice": {"client": {"host": "localhost", "port": 8081}},
            "service": {"name": "uri-shortener", "environment": "development"}
        },
        "runtime": {
            "load_shedder": {"max_concurrent_requests": 10000}
        }
    })";

  auto result = ProtoConfigLoader::loadFromString(json);

  ASSERT_TRUE(result.is_ok()) << result.error();
  EXPECT_EQ(result.value().schema_version(), 1);
  EXPECT_EQ(result.value().bootstrap().server().uri(), "0.0.0.0:8080");
  EXPECT_EQ(result.value().bootstrap().service().name(), "uri-shortener");
  EXPECT_EQ(result.value().runtime().load_shedder().max_concurrent_requests(),
            10000);
}

} // namespace uri_shortener::test
