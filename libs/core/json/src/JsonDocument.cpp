#include "JsonDocument.h"

#include <boost/json.hpp>
#include <boost/system/error_code.hpp>
#include <stdexcept>

namespace astra::json {

class JsonDocument::Impl {
public:
  boost::json::value val;

  Impl() : val(boost::json::object()) {
  }
  explicit Impl(boost::json::value v) : val(std::move(v)) {
  }
};

JsonDocument::JsonDocument() : m_impl(std::make_unique<Impl>()) {
}

JsonDocument::JsonDocument(std::unique_ptr<Impl> impl)
    : m_impl(std::move(impl)) {
}

JsonDocument::~JsonDocument() = default;

JsonDocument::JsonDocument(JsonDocument &&) noexcept = default;
JsonDocument &JsonDocument::operator=(JsonDocument &&) noexcept = default;

JsonDocument JsonDocument::parse(const std::string &json_str) {
  boost::system::error_code ec;
  boost::json::value val = boost::json::parse(json_str, ec);
  if (ec) {
    throw std::runtime_error("JSON parse error: " + ec.message());
  }
  return JsonDocument(std::make_unique<Impl>(std::move(val)));
}

bool JsonDocument::contains(const std::string &key) const {
  if (!m_impl->val.is_object()) {
    return false;
  }
  return m_impl->val.as_object().contains(key);
}

std::string JsonDocument::get_string(const std::string &key) const {
  if (!contains(key)) {
    throw std::runtime_error("Key not found: " + key);
  }
  const auto &v = m_impl->val.as_object().at(key);
  if (!v.is_string()) {
    throw std::runtime_error("Value is not a string: " + key);
  }
  return std::string(v.as_string().c_str());
}

int JsonDocument::get_int(const std::string &key) const {
  if (!contains(key)) {
    throw std::runtime_error("Key not found: " + key);
  }
  const auto &v = m_impl->val.as_object().at(key);
  if (!v.is_int64()) {
    throw std::runtime_error("Value is not an integer: " + key);
  }
  return static_cast<int>(v.as_int64());
}

uint64_t JsonDocument::get_uint64(const std::string &key) const {
  if (!contains(key)) {
    throw std::runtime_error("Key not found: " + key);
  }
  const auto &v = m_impl->val.as_object().at(key);
  if (v.is_uint64()) {
    return v.as_uint64();
  }
  if (v.is_int64() && v.as_int64() >= 0) {
    return static_cast<uint64_t>(v.as_int64());
  }
  throw std::runtime_error("Value is not a uint64: " + key);
}

bool JsonDocument::get_bool(const std::string &key) const {
  if (!contains(key)) {
    throw std::runtime_error("Key not found: " + key);
  }
  const auto &v = m_impl->val.as_object().at(key);
  if (!v.is_bool()) {
    throw std::runtime_error("Value is not a boolean: " + key);
  }
  return v.as_bool();
}

double JsonDocument::get_double(const std::string &key) const {
  if (!contains(key)) {
    throw std::runtime_error("Key not found: " + key);
  }
  const auto &v = m_impl->val.as_object().at(key);
  if (v.is_double()) {
    return v.as_double();
  }
  if (v.is_int64()) {
    return static_cast<double>(v.as_int64());
  }
  if (v.is_uint64()) {
    return static_cast<double>(v.as_uint64());
  }
  throw std::runtime_error("Value is not a number: " + key);
}

JsonDocument JsonDocument::get_child(const std::string &key) const {
  if (!contains(key)) {
    throw std::runtime_error("Key not found: " + key);
  }
  boost::json::value child = m_impl->val.as_object().at(key);
  return JsonDocument(std::make_unique<Impl>(std::move(child)));
}

bool JsonDocument::is_object() const {
  return m_impl->val.is_object();
}
bool JsonDocument::is_array() const {
  return m_impl->val.is_array();
}
bool JsonDocument::is_string() const {
  return m_impl->val.is_string();
}
bool JsonDocument::is_number() const {
  return m_impl->val.is_number();
}
bool JsonDocument::is_bool() const {
  return m_impl->val.is_bool();
}
bool JsonDocument::is_null() const {
  return m_impl->val.is_null();
}

} // namespace astra::json
