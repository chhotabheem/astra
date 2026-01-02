#pragma once

#include <string>
#include <unordered_map>

namespace astra::router {

class IRequest {
public:
  virtual ~IRequest() = default;

  [[nodiscard]] virtual const std::string &method() const = 0;
  [[nodiscard]] virtual const std::string &path() const = 0;
  [[nodiscard]] virtual std::string header(const std::string &key) const = 0;
  [[nodiscard]] virtual const std::string &body() const = 0;

  [[nodiscard]] virtual std::string
  path_param(const std::string &key) const = 0;
  [[nodiscard]] virtual std::string
  query_param(const std::string &key) const = 0;

  virtual void
  set_path_params(std::unordered_map<std::string, std::string> params) = 0;
};

} // namespace astra::router
