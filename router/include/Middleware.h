#pragma once

#include "IRequest.h"
#include "IResponse.h"
#include <functional>

namespace router {

using Next = std::function<void()>;
using Middleware = std::function<void(router::IRequest&, router::IResponse&, Next)>;
using Handler = std::function<void(router::IRequest&, router::IResponse&)>;

} // namespace router
