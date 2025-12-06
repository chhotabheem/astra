#include "JsonWriter.h"
#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <vector>
#include <utility>

namespace json {

class JsonWriter::Impl {
public:
    Impl() {
        // Push root object
        stack.emplace_back("", boost::json::object());
    }

    // Stack of (key_in_parent, object_being_built)
    // For the root object, key is empty.
    std::vector<std::pair<std::string, boost::json::object>> stack;
};

JsonWriter::JsonWriter() : m_impl(std::make_unique<Impl>()) {}

JsonWriter::~JsonWriter() = default;

JsonWriter::JsonWriter(JsonWriter&&) noexcept = default;
JsonWriter& JsonWriter::operator=(JsonWriter&&) noexcept = default;

void JsonWriter::add(std::string_view key, std::string_view value) {
    m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(std::string_view key, const std::string& value) {
    m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(std::string_view key, const char* value) {
    m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(std::string_view key, int value) {
    m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(std::string_view key, long value) {
    m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(std::string_view key, unsigned long value) {
    m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(std::string_view key, bool value) {
    m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(std::string_view key, double value) {
    m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::start_object(std::string_view key) {
    m_impl->stack.emplace_back(std::string(key), boost::json::object());
}

void JsonWriter::end_object() {
    if (m_impl->stack.size() > 1) {
        auto child = std::move(m_impl->stack.back());
        m_impl->stack.pop_back();
        m_impl->stack.back().second.emplace(child.first, std::move(child.second));
    }
    // If stack size is 1, it's the root object closing. We keep it to serialize later.
}

std::string JsonWriter::get_string() const {
    if (m_impl->stack.empty()) {
        return "{}";
    }
    // The root object is always at index 0
    return boost::json::serialize(m_impl->stack.front().second);
}

} // namespace json
