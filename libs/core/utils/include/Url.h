#pragma once

#include <string>
#include <unordered_map>

namespace astra::utils {

class Url {
public:
  Url() = delete;

  [[nodiscard]] static std::unordered_map<std::string, std::string>
  parse_query_string(const std::string &query_string);
};

} // namespace astra::utils
