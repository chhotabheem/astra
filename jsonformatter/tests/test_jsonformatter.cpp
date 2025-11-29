#include "JsonFormatter.h"
#include <iostream>
#include <cassert>
#include <string>

using namespace json;

void test_basic_json() {
    JsonFormatter json;
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
    JsonFormatter json;
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
    JsonFormatter json;
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
    JsonFormatter json;
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
    
    assert(result.find("\"timestamp\":\"2025-11-23T18:47:00.123456\"") != std::string::npos);
    assert(result.find("\"level\":\"INFO\"") != std::string::npos);
    assert(result.find("\"message\":\"Application started\"") != std::string::npos);
    assert(result.find("\"source\":{") != std::string::npos);
    assert(result.find("\"file\":\"main.cpp\"") != std::string::npos);
    assert(result.find("\"line\":42") != std::string::npos);
    
    std::cout << "✓ Logger use case test passed\n";
    std::cout << "  Result: " << result << "\n";
}

int main() {
    std::cout << "Running JsonFormatter tests...\n\n";
    
    try {
        test_basic_json();
        test_nested_object();
        test_various_types();
        test_logger_use_case();
        
        std::cout << "\n✅ All tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
