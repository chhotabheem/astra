#pragma once

#include <string>
#include <vector>

namespace astra::utils {

/// Split a string by a delimiter character, skipping empty segments
/// @param str The string to split
/// @param delimiter The character to split on
/// @return Vector of non-empty segments
[[nodiscard]] std::vector<std::string> split(const std::string &str,
                                             char delimiter);

} // namespace astra::utils
