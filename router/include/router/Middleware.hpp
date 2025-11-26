#pragma once

#include "http_abstractions/IRequest.hpp"
#include "http_abstractions/IResponse.hpp"
#include <functional>

namespace router {

using Next = std::function<void()>;
using Middleware = std::function<void(const http_abstractions::IRequest&, http_abstractions::IResponse&, Next)>;
using Handler = std::function<void(const http_abstractions::IRequest&, http_abstractions::IResponse&)>;

} // namespace router
