#include "JsonWriter.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace astra::json;

void test_basic_json() {
  JsonWriter json;
  json.add("name", "Alice");
  json.add("age", 30);
  json.add("active", true);
  json.end_object();

  std::string result = json.get_string();

  assert(result.find("\"name\":\"Alice\"") != std::string::npos);
  assert(result.find("\"age\":30") != std::string::npos);
  assert(result.find("\"active\":true") != std::string::npos);

  std::cout << "✓ Basic JSON test passed\n";
}

void test_nested_object() {
  JsonWriter json;
  json.add("user", "Bob");
  json.start_object("address");
  json.add("city", "NYC");
  json.add("zip", 10001);
  json.end_object();
  json.end_object();

  std::string result = json.get_string();

  assert(result.find("\"user\":\"Bob\"") != std::string::npos);
  assert(result.find("\"address\":{") != std::string::npos);
  assert(result.find("\"city\":\"NYC\"") != std::string::npos);
  assert(result.find("\"zip\":10001") != std::string::npos);

  std::cout << "✓ Nested object test passed\n";
  std::cout << "  Result: " << result << "\n";
}

void test_various_types() {
  JsonWriter json;
  json.add("string", "test");
  json.add("int", 42);
  json.add("long", 9223372036854775807L);
  json.add("ulong", 18446744073709551615UL);
  json.add("bool_true", true);
  json.add("bool_false", false);
  json.add("double", 3.14159);
  json.end_object();

  std::string result = json.get_string();

  assert(result.find("\"string\":\"test\"") != std::string::npos);
  assert(result.find("\"int\":42") != std::string::npos);
  assert(result.find("\"bool_true\":true") != std::string::npos);
  assert(result.find("\"bool_false\":false") != std::string::npos);
  assert(result.find("\"double\":3.14159") != std::string::npos);

  std::cout << "✓ Various types test passed\n";
  std::cout << "  Result: " << result << "\n";
}

void test_logger_use_case() {
  JsonWriter json;
  json.add("timestamp", "2025-11-23T18:47:00.123456");
  json.add("level", "INFO");
  json.add("message", "Application started");
  json.start_object("source");
  json.add("file", "main.cpp");
  json.add("line", 42);
  json.add("function", "main");
  json.end_object();
  json.add("thread_id", "12345");
  json.end_object();

  std::string result = json.get_string();

  assert(result.find("\"timestamp\":\"2025-11-23T18:47:00.123456\"") !=
         std::string::npos);
  assert(result.find("\"level\":\"INFO\"") != std::string::npos);
  assert(result.find("\"message\":\"Application started\"") !=
         std::string::npos);
  assert(result.find("\"source\":{") != std::string::npos);
  assert(result.find("\"file\":\"main.cpp\"") != std::string::npos);
  assert(result.find("\"line\":42") != std::string::npos);

  std::cout << "✓ Logger use case test passed\n";
  std::cout << "  Result: " << result << "\n";
}

void test_special_characters() {
  JsonWriter json;
  json.add("quote", "He said \"Hello\"");
  json.add("newline", "Line 1\nLine 2");
  json.add("tab", "Column1\tColumn2");
  json.add("backslash", "C:\\Windows\\System32");
  json.end_object();

  std::string result = json.get_string();

  // Check for escaped characters
  assert(result.find("\\\"Hello\\\"") != std::string::npos);
  assert(result.find("\\n") != std::string::npos);
  assert(result.find("\\t") != std::string::npos);
  assert(result.find("C:\\\\Windows\\\\System32") != std::string::npos);

  std::cout << "✓ Special characters test passed\n";
}

void test_empty_strings() {
  JsonWriter json;
  json.add("", "");
  json.add("empty_key", "");
  json.add("", "empty_value");
  json.end_object();

  std::string result = json.get_string();

  assert(result.find("\"\":\"\"") != std::string::npos);
  assert(result.find("\"empty_key\":\"\"") != std::string::npos);

  std::cout << "✓ Empty strings test passed\n";
}

void test_deep_nesting() {
  JsonWriter json;
  json.start_object("level1");
  json.start_object("level2");
  json.start_object("level3");
  json.add("value", "deep");
  json.end_object();
  json.end_object();
  json.end_object();
  json.end_object(); // Close root

  std::string result = json.get_string();

  assert(result.find(
             "\"level1\":{\"level2\":{\"level3\":{\"value\":\"deep\"}}}") !=
         std::string::npos);

  std::cout << "✓ Deep nesting test passed\n";
}

int main() {
  std::cout << "Running JsonWriter tests...\n\n";

  try {
    test_basic_json();
    test_nested_object();
    test_various_types();
    test_logger_use_case();
    test_special_characters();
    test_empty_strings();
    test_deep_nesting();

    std::cout << "\n✅ All tests passed!\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n";
    return 1;
  }
}
