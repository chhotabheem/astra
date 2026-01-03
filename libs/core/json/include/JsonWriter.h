#pragma once

#include <memory>
#include <string>

namespace astra::json {

class JsonWriter {
public:
  JsonWriter();
  ~JsonWriter();

  JsonWriter(const JsonWriter &) = delete;
  JsonWriter &operator=(const JsonWriter &) = delete;
  JsonWriter(JsonWriter &&) noexcept;
  JsonWriter &operator=(JsonWriter &&) noexcept;

  void add(const std::string &key, const std::string &value);
  void add(const std::string &key, const char *value);
  void add(const std::string &key, int value);
  void add(const std::string &key, long value);
  void add(const std::string &key, unsigned long value);
  void add(const std::string &key, bool value);
  void add(const std::string &key, double value);

  void start_object(const std::string &key);
  void end_object();

  std::string get_string() const;

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace astra::json
