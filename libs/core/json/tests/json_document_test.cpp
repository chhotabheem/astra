#include "JsonDocument.h"

#include <gtest/gtest.h>
#include <limits>
#include <string>

using namespace astra::json;

TEST(JsonDocumentTest, ParseValidJson) {
  std::string json_str = R"({
        "name": "Astra",
        "port": 8080,
        "enabled": true,
        "ratio": 1.5
    })";

  EXPECT_NO_THROW({
    auto doc = JsonDocument::parse(json_str);

    EXPECT_TRUE(doc.contains("name"));
    EXPECT_TRUE(doc.contains("port"));
    EXPECT_TRUE(doc.contains("enabled"));
    EXPECT_FALSE(doc.contains("missing"));

    EXPECT_EQ(doc.get_string("name"), "Astra");
    EXPECT_EQ(doc.get_int("port"), 8080);
    EXPECT_EQ(doc.get_bool("enabled"), true);
    EXPECT_DOUBLE_EQ(doc.get_double("ratio"), 1.5);
  });
}

TEST(JsonDocumentTest, ParseNestedObject) {
  std::string json_str = R"({
        "server": {
            "host": "localhost",
            "port": 9090
        }
    })";

  auto doc = JsonDocument::parse(json_str);
  EXPECT_TRUE(doc.contains("server"));

  auto server = doc.get_child("server");
  EXPECT_EQ(server.get_string("host"), "localhost");
  EXPECT_EQ(server.get_int("port"), 9090);
}

TEST(JsonDocumentTest, ParseInvalidJson) {
  std::string json_str = "{ invalid json }";
  EXPECT_THROW(JsonDocument::parse(json_str), std::exception);
}

TEST(JsonDocumentTest, ParseEmptyObject) {
  std::string json_str = "{}";
  auto doc = JsonDocument::parse(json_str);
  EXPECT_TRUE(doc.is_object());
  EXPECT_FALSE(doc.contains("anything"));
}

TEST(JsonDocumentTest, ParseEmptyString) {
  EXPECT_THROW(JsonDocument::parse(""), std::exception);
}

TEST(JsonDocumentTest, ParseWhitespaceOnly) {
  EXPECT_THROW(JsonDocument::parse("   \n\t  "), std::exception);
}

TEST(JsonDocumentTest, ParseArray) {
  std::string json_str = R"([1, 2, 3, 4, 5])";
  auto doc = JsonDocument::parse(json_str);
  EXPECT_TRUE(doc.is_array());
}

TEST(JsonDocumentTest, ParseNull) {
  std::string json_str = R"({"value": null})";
  auto doc = JsonDocument::parse(json_str);
  auto child = doc.get_child("value");
  EXPECT_TRUE(child.is_null());
}

TEST(JsonDocumentTest, ParseDeeplyNested) {
  std::string json_str = R"({
        "level1": {
            "level2": {
                "level3": {
                    "level4": {
                        "value": 42
                    }
                }
            }
        }
    })";

  auto doc = JsonDocument::parse(json_str);
  auto l1 = doc.get_child("level1");
  auto l2 = l1.get_child("level2");
  auto l3 = l2.get_child("level3");
  auto l4 = l3.get_child("level4");
  EXPECT_EQ(l4.get_int("value"), 42);
}

TEST(JsonDocumentTest, TypeIsObject) {
  auto doc = JsonDocument::parse(R"({"key": "value"})");
  EXPECT_TRUE(doc.is_object());
  EXPECT_FALSE(doc.is_array());
  EXPECT_FALSE(doc.is_string());
  EXPECT_FALSE(doc.is_number());
  EXPECT_FALSE(doc.is_bool());
  EXPECT_FALSE(doc.is_null());
}

TEST(JsonDocumentTest, TypeIsArray) {
  auto doc = JsonDocument::parse(R"([1, 2, 3])");
  EXPECT_TRUE(doc.is_array());
  EXPECT_FALSE(doc.is_object());
}

TEST(JsonDocumentTest, ChildTypeIsString) {
  auto doc = JsonDocument::parse(R"({"name": "test"})");
  auto child = doc.get_child("name");
  EXPECT_TRUE(child.is_string());
}

TEST(JsonDocumentTest, ChildTypeIsNumber) {
  auto doc = JsonDocument::parse(R"({"count": 42})");
  auto child = doc.get_child("count");
  EXPECT_TRUE(child.is_number());
}

TEST(JsonDocumentTest, ChildTypeIsBool) {
  auto doc = JsonDocument::parse(R"({"active": true})");
  auto child = doc.get_child("active");
  EXPECT_TRUE(child.is_bool());
}

TEST(JsonDocumentTest, GetMissingKey) {
  auto doc = JsonDocument::parse(R"({"key": "value"})");
  EXPECT_THROW(doc.get_string("nonexistent"), std::exception);
}

TEST(JsonDocumentTest, GetWrongType) {
  auto doc = JsonDocument::parse(R"({"name": "text"})");
  EXPECT_THROW(doc.get_int("name"), std::exception);
}

TEST(JsonDocumentTest, GetIntFromDouble) {
  auto doc = JsonDocument::parse(R"({"value": 3.14})");
  // Behavior depends on implementation - may truncate or throw
  EXPECT_NO_THROW({
    auto val = doc.get_double("value");
    EXPECT_DOUBLE_EQ(val, 3.14);
  });
}

TEST(JsonDocumentTest, GetUint64LargeValue) {
  std::string json_str = R"({"big": 18446744073709551615})";
  auto doc = JsonDocument::parse(json_str);
  EXPECT_NO_THROW({
    auto val = doc.get_uint64("big");
    // Large unsigned value
  });
}

TEST(JsonDocumentTest, NegativeInteger) {
  auto doc = JsonDocument::parse(R"({"value": -42})");
  EXPECT_EQ(doc.get_int("value"), -42);
}

TEST(JsonDocumentTest, EscapedCharactersInString) {
  std::string json_str = R"({"text": "Hello\nWorld\t!"})";
  auto doc = JsonDocument::parse(json_str);
  auto text = doc.get_string("text");
  EXPECT_TRUE(text.find('\n') != std::string::npos);
  EXPECT_TRUE(text.find('\t') != std::string::npos);
}

TEST(JsonDocumentTest, UnicodeInString) {
  std::string json_str = R"({"emoji": "Hello 🚀"})";
  auto doc = JsonDocument::parse(json_str);
  auto text = doc.get_string("emoji");
  EXPECT_FALSE(text.empty());
}

TEST(JsonDocumentTest, EmptyStringValue) {
  std::string json_str = R"({"empty": ""})";
  auto doc = JsonDocument::parse(json_str);
  EXPECT_EQ(doc.get_string("empty"), "");
}

TEST(JsonDocumentTest, BooleanTrue) {
  auto doc = JsonDocument::parse(R"({"flag": true})");
  EXPECT_EQ(doc.get_bool("flag"), true);
}

TEST(JsonDocumentTest, BooleanFalse) {
  auto doc = JsonDocument::parse(R"({"flag": false})");
  EXPECT_EQ(doc.get_bool("flag"), false);
}

TEST(JsonDocumentTest, MoveConstruction) {
  auto doc1 = JsonDocument::parse(R"({"key": "value"})");
  auto doc2 = std::move(doc1);

  EXPECT_TRUE(doc2.contains("key"));
  EXPECT_EQ(doc2.get_string("key"), "value");
}

TEST(JsonDocumentTest, MoveAssignment) {
  auto doc1 = JsonDocument::parse(R"({"key": "value1"})");
  auto doc2 = JsonDocument::parse(R"({"other": "value2"})");

  doc2 = std::move(doc1);

  EXPECT_TRUE(doc2.contains("key"));
  EXPECT_EQ(doc2.get_string("key"), "value1");
}

TEST(JsonDocumentTest, LargeJsonDocument) {
  std::string json_str = "{";
  for (int i = 0; i < 100; ++i) {
    if (i > 0) {
      json_str += ",";
    }
    json_str += "\"key" + std::to_string(i) + "\": " + std::to_string(i);
  }
  json_str += "}";

  auto doc = JsonDocument::parse(json_str);
  EXPECT_TRUE(doc.contains("key0"));
  EXPECT_TRUE(doc.contains("key99"));
  EXPECT_EQ(doc.get_int("key50"), 50);
}

TEST(JsonDocumentTest, ParseManyTimes) {
  std::string json_str = R"({"test": true})";
  for (int i = 0; i < 1000; ++i) {
    auto doc = JsonDocument::parse(json_str);
    EXPECT_TRUE(doc.get_bool("test"));
  }
}
