#include "JsonFormatter.h"
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace json {

class JsonFormatter::Impl {
public:
    Impl() : writer(buffer) {
        writer.StartObject();
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer;
};

JsonFormatter::JsonFormatter() : m_impl(std::make_unique<Impl>()) {}

JsonFormatter::~JsonFormatter() = default;

JsonFormatter::JsonFormatter(JsonFormatter&&) noexcept = default;
JsonFormatter& JsonFormatter::operator=(JsonFormatter&&) noexcept = default;

void JsonFormatter::add(std::string_view key, std::string_view value) {
    m_impl->writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
    m_impl->writer.String(value.data(), static_cast<rapidjson::SizeType>(value.length()));
}

void JsonFormatter::add(std::string_view key, const std::string& value) {
    m_impl->writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
    m_impl->writer.String(value.c_str(), static_cast<rapidjson::SizeType>(value.length()));
}

void JsonFormatter::add(std::string_view key, const char* value) {
    m_impl->writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
    m_impl->writer.String(value);
}

void JsonFormatter::add(std::string_view key, int value) {
    m_impl->writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
    m_impl->writer.Int(value);
}

void JsonFormatter::add(std::string_view key, long value) {
    m_impl->writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
    m_impl->writer.Int64(value);
}

void JsonFormatter::add(std::string_view key, unsigned long value) {
    m_impl->writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
    m_impl->writer.Uint64(value);
}

void JsonFormatter::add(std::string_view key, bool value) {
    m_impl->writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
    m_impl->writer.Bool(value);
}

void JsonFormatter::add(std::string_view key, double value) {
    m_impl->writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
    m_impl->writer.Double(value);
}

void JsonFormatter::start_object(std::string_view key) {
    m_impl->writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.length()));
    m_impl->writer.StartObject();
}

void JsonFormatter::end_object() {
    m_impl->writer.EndObject();
}

std::string JsonFormatter::get_string() const {
    return m_impl->buffer.GetString();
}

} // namespace json
