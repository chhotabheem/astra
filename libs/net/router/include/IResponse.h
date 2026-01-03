#pragma once

#include <string>

namespace astra::router {

class IResponse {
public:
  virtual ~IResponse() = default;

  virtual void set_status(int code) noexcept = 0;
  virtual void set_header(const std::string &key, const std::string &value) = 0;
  virtual void write(const std::string &data) = 0;
  virtual void close() = 0;

  [[nodiscard]] virtual bool is_alive() const noexcept = 0;
};

} // namespace astra::router
