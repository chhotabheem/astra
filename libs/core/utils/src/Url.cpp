#include "Url.h"

#include <boost/url/parse_query.hpp>

namespace astra::utils {

std::unordered_map<std::string, std::string>
Url::parse_query_string(const std::string &query_string) {
  std::unordered_map<std::string, std::string> result;

  if (query_string.empty()) {
    return result;
  }

  auto parsed = boost::urls::parse_query(query_string);
  if (parsed) {
    for (const auto &param : *parsed) {
      result[param.key.decode()] = param.value.decode();
    }
  }
  return result;
}

} // namespace astra::utils
