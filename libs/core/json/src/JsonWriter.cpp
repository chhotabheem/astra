#include "JsonWriter.h"

#include <boost/json.hpp>
#include <utility>
#include <vector>

namespace astra::json {

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

JsonWriter::JsonWriter() : m_impl(std::make_unique<Impl>()) {
}

JsonWriter::~JsonWriter() = default;

JsonWriter::JsonWriter(JsonWriter &&) noexcept = default;
JsonWriter &JsonWriter::operator=(JsonWriter &&) noexcept = default;

void JsonWriter::add(const std::string &key, const std::string &value) {
  m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(const std::string &key, const char *value) {
  m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(const std::string &key, int value) {
  m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(const std::string &key, long value) {
  m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(const std::string &key, unsigned long value) {
  m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(const std::string &key, bool value) {
  m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::add(const std::string &key, double value) {
  m_impl->stack.back().second.emplace(key, value);
}

void JsonWriter::start_object(const std::string &key) {
  m_impl->stack.emplace_back(key, boost::json::object());
}

void JsonWriter::end_object() {
  if (m_impl->stack.size() > 1) {
    auto child = std::move(m_impl->stack.back());
    m_impl->stack.pop_back();
    m_impl->stack.back().second.emplace(child.first, std::move(child.second));
  }
  // If stack size is 1, it's the root object closing. We keep it to serialize
  // later.
}

std::string JsonWriter::get_string() const {
  if (m_impl->stack.empty()) {
    return "{}";
  }
  // The root object is always at index 0
  return boost::json::serialize(m_impl->stack.front().second);
}

} // namespace astra::json
