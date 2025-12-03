#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <optional>

namespace json {

class JsonDocument {
public:
    JsonDocument();
    ~JsonDocument();

    // Move-only type
    JsonDocument(const JsonDocument&) = delete;
    JsonDocument& operator=(const JsonDocument&) = delete;
    JsonDocument(JsonDocument&&) noexcept;
    JsonDocument& operator=(JsonDocument&&) noexcept;

    // Static factory
    static JsonDocument parse(const std::string& json_str);

    // Accessors
    bool contains(const std::string& key) const;
    std::string get_string(const std::string& key) const;
    int get_int(const std::string& key) const;
    uint64_t get_uint64(const std::string& key) const;
    bool get_bool(const std::string& key) const;
    double get_double(const std::string& key) const;
    
    // Nested access
    JsonDocument get_child(const std::string& key) const;
    
    // Type checks
    bool is_object() const;
    bool is_array() const;
    bool is_string() const;
    bool is_number() const;
    bool is_bool() const;
    bool is_null() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    
    // Private constructor for internal use
    explicit JsonDocument(std::unique_ptr<Impl> impl);
};

} // namespace json
