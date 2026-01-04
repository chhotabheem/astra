#include "JsonDocument.h"
#include "fuzztest/fuzztest.h"

#include "gtest/gtest.h"
#include <string>

using namespace astra::json;

void ParseNeverCrashes(const std::string &input) {
  try {
    auto doc = JsonDocument::parse(input);
  } catch (const std::exception &) {
    // Expected for invalid JSON - just don't crash
  }
}
FUZZ_TEST(JsonFuzzTest, ParseNeverCrashes);

void ParseKeyValuePair(const std::string &key, int value) {
  if (key.empty() || key.find('"') != std::string::npos) {
    return; // Skip problematic keys
  }

  std::string json = "{\"" + key + "\": " + std::to_string(value) + "}";
  try {
    auto doc = JsonDocument::parse(json);
    if (doc.contains(key)) {
      doc.get_int(key); // Should not crash
    }
  } catch (const std::exception &) {
    // Allowed
  }
}
FUZZ_TEST(JsonFuzzTest, ParseKeyValuePair);

void ParseNestedJson(int depth) {
  if (depth < 0 || depth > 100) {
    return; // Limit depth
  }

  std::string json = "";
  for (int i = 0; i < depth; ++i) {
    json += "{\"level" + std::to_string(i) + "\": ";
  }
  json += "true";
  for (int i = 0; i < depth; ++i) {
    json += "}";
  }

  try {
    auto doc = JsonDocument::parse(json);
  } catch (const std::exception &) {
    // Allowed
  }
}
FUZZ_TEST(JsonFuzzTest, ParseNestedJson);

void ParseStringValue(const std::string &value) {
  // Escape the string value for JSON
  std::string escaped;
  for (char c : value) {
    if (c == '"') {
      escaped += "\\\"";
    } else if (c == '\\') {
      escaped += "\\\\";
    } else if (c == '\n') {
      escaped += "\\n";
    } else if (c == '\r') {
      escaped += "\\r";
    } else if (c == '\t') {
      escaped += "\\t";
    } else if (c >= 0 && c < 32) {
      continue; // Skip control chars
    } else {
      escaped += c;
    }
  }

  std::string json = "{\"key\": \"" + escaped + "\"}";
  try {
    auto doc = JsonDocument::parse(json);
  } catch (const std::exception &) {
    // Allowed
  }
}
FUZZ_TEST(JsonFuzzTest, ParseStringValue);
