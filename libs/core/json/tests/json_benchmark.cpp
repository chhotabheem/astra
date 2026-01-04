#include "JsonDocument.h"
#include "JsonWriter.h"

#include <benchmark/benchmark.h>
#include <string>

using namespace astra::json;

static const std::string small_json = R"({"name":"test","value":42})";

static const std::string medium_json = R"({
    "user": {
        "id": 12345,
        "name": "John Doe",
        "email": "john@example.com",
        "active": true
    },
    "items": [1, 2, 3, 4, 5],
    "metadata": {
        "created": "2024-01-01",
        "version": "1.0.0"
    }
})";

static const std::string large_json = []() {
  std::string json = "{\"items\": [";
  for (int i = 0; i < 100; ++i) {
    if (i > 0) {
      json += ",";
    }
    json += "{\"id\":" + std::to_string(i) + ",\"name\":\"item" +
            std::to_string(i) + "\"}";
  }
  json += "]}";
  return json;
}();

static void BM_ParseSmall(benchmark::State &state) {
  for (auto _ : state) {
    auto doc = JsonDocument::parse(small_json);
    benchmark::DoNotOptimize(doc);
  }
}
BENCHMARK(BM_ParseSmall);

static void BM_ParseMedium(benchmark::State &state) {
  for (auto _ : state) {
    auto doc = JsonDocument::parse(medium_json);
    benchmark::DoNotOptimize(doc);
  }
}
BENCHMARK(BM_ParseMedium);

static void BM_ParseLarge(benchmark::State &state) {
  for (auto _ : state) {
    auto doc = JsonDocument::parse(large_json);
    benchmark::DoNotOptimize(doc);
  }
}
BENCHMARK(BM_ParseLarge);

static void BM_GetString(benchmark::State &state) {
  auto doc = JsonDocument::parse(medium_json);
  auto user = doc.get_child("user");

  for (auto _ : state) {
    auto name = user.get_string("name");
    benchmark::DoNotOptimize(name);
  }
}
BENCHMARK(BM_GetString);

static void BM_GetInt(benchmark::State &state) {
  auto doc = JsonDocument::parse(medium_json);
  auto user = doc.get_child("user");

  for (auto _ : state) {
    auto id = user.get_int("id");
    benchmark::DoNotOptimize(id);
  }
}
BENCHMARK(BM_GetInt);

static void BM_Contains(benchmark::State &state) {
  auto doc = JsonDocument::parse(medium_json);

  for (auto _ : state) {
    bool has = doc.contains("user");
    benchmark::DoNotOptimize(has);
  }
}
BENCHMARK(BM_Contains);

static void BM_WriteSmall(benchmark::State &state) {
  for (auto _ : state) {
    JsonWriter writer;
    writer.add("name", "test");
    writer.add("value", 42);
    std::string result = writer.get_string();
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_WriteSmall);

static void BM_WriteMedium(benchmark::State &state) {
  for (auto _ : state) {
    JsonWriter writer;
    writer.start_object("user");
    writer.add("id", 12345);
    writer.add("name", "John Doe");
    writer.add("email", "john@example.com");
    writer.add("active", true);
    writer.end_object();
    std::string result = writer.get_string();
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_WriteMedium);

BENCHMARK_MAIN();
