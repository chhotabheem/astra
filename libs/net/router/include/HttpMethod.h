#pragma once

#include <string>

namespace astra::router {

enum class HttpMethod { GET, POST, PUT, DELETE };

std::string to_string(HttpMethod method);

} // namespace astra::router
