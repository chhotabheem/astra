#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace astra::json {

class JsonDocument {
public:
  JsonDocument();
  ~JsonDocument();

  JsonDocument(const JsonDocument &) = delete;
  JsonDocument &operator=(const JsonDocument &) = delete;
  JsonDocument(JsonDocument &&) noexcept;
  JsonDocument &operator=(JsonDocument &&) noexcept;

  [[nodiscard]] static JsonDocument parse(const std::string &json_str);

  [[nodiscard]] bool contains(const std::string &key) const;
  [[nodiscard]] std::string get_string(const std::string &key) const;
  [[nodiscard]] int get_int(const std::string &key) const;
  [[nodiscard]] uint64_t get_uint64(const std::string &key) const;
  [[nodiscard]] bool get_bool(const std::string &key) const;
  [[nodiscard]] double get_double(const std::string &key) const;

  [[nodiscard]] JsonDocument get_child(const std::string &key) const;

  [[nodiscard]] bool is_object() const;
  [[nodiscard]] bool is_array() const;
  [[nodiscard]] bool is_string() const;
  [[nodiscard]] bool is_number() const;
  [[nodiscard]] bool is_bool() const;
  [[nodiscard]] bool is_null() const;

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;

  explicit JsonDocument(std::unique_ptr<Impl> impl);
};

} // namespace astra::json
