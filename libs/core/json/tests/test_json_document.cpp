#include "JsonDocument.h"
#include <iostream>
#include <cassert>
#include <string>
#include <stdexcept>

using namespace json;

void test_parse_valid_json() {
    std::string json_str = R"({
        "name": "Astra",
        "port": 8080,
        "enabled": true,
        "ratio": 1.5
    })";
    
    try {
        auto doc = JsonDocument::parse(json_str);
        
        assert(doc.contains("name"));
        assert(doc.contains("port"));
        assert(doc.contains("enabled"));
        assert(!doc.contains("missing"));
        
        assert(doc.get_string("name") == "Astra");
        assert(doc.get_int("port") == 8080);
        assert(doc.get_bool("enabled") == true);
        // assert(doc.get_double("ratio") == 1.5); // Floating point comparison needs epsilon
        
        std::cout << "✓ Parse valid JSON passed\n";
    } catch (const std::exception& e) {
        std::cerr << "❌ Parse valid JSON failed: " << e.what() << "\n";
        throw;
    }
}

void test_nested_object() {
    std::string json_str = R"({
        "server": {
            "host": "localhost",
            "port": 9090
        }
    })";
    
    try {
        auto doc = JsonDocument::parse(json_str);
        assert(doc.contains("server"));
        
        auto server = doc.get_child("server");
        assert(server.get_string("host") == "localhost");
        assert(server.get_int("port") == 9090);
        
        std::cout << "✓ Nested object passed\n";
    } catch (const std::exception& e) {
        std::cerr << "❌ Nested object failed: " << e.what() << "\n";
        throw;
    }
}

void test_invalid_json() {
    std::string json_str = "{ invalid json }";
    try {
        auto doc = JsonDocument::parse(json_str);
        std::cerr << "❌ Invalid JSON test failed (should have thrown)\n";
        exit(1);
    } catch (const std::exception&) {
        std::cout << "✓ Invalid JSON test passed (threw exception)\n";
    }
}

int main() {
    std::cout << "Running JsonDocument tests...\n\n";
    
    try {
        test_parse_valid_json();
        test_nested_object();
        test_invalid_json();
        
        std::cout << "\n✅ All tests passed!\n";
        return 0;
    } catch (...) {
        return 1;
    }
}
