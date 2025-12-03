#pragma once

#include <string>
#include <string_view>
#include <memory>

namespace json {

class JsonWriter {
public:
    JsonWriter();
    ~JsonWriter();

    JsonWriter(const JsonWriter&) = delete;
    JsonWriter& operator=(const JsonWriter&) = delete;
    JsonWriter(JsonWriter&&) noexcept;
    JsonWriter& operator=(JsonWriter&&) noexcept;

    void add(std::string_view key, std::string_view value);
    void add(std::string_view key, const std::string& value);
    void add(std::string_view key, const char* value);
    void add(std::string_view key, int value);
    void add(std::string_view key, long value);
    void add(std::string_view key, unsigned long value);
    void add(std::string_view key, bool value);
    void add(std::string_view key, double value);

    void start_object(std::string_view key);
    void end_object();

    std::string get_string() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace json
