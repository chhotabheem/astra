#include "StringUtils.h"

namespace astra::utils {

std::vector<std::string> split(const std::string &str, char delimiter) {
  std::vector<std::string> segments;
  size_t start = 0;

  for (size_t i = 0; i < str.length(); ++i) {
    if (str[i] == delimiter) {
      if (i > start) {
        segments.push_back(str.substr(start, i - start));
      }
      start = i + 1;
    }
  }

  if (start < str.length()) {
    segments.push_back(str.substr(start));
  }

  return segments;
}

} // namespace astra::utils
