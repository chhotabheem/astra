#pragma once

#include "HttpMethod.h"
#include "IRequest.h"
#include "IResponse.h"

#include <functional>
#include <memory>
#include <string>

namespace astra::router {

using Handler =
    std::function<void(std::shared_ptr<IRequest>, std::shared_ptr<IResponse>)>;

class IRouter {
public:
  virtual ~IRouter() = default;
  virtual void add(HttpMethod method, const std::string &path,
                   Handler handler) = 0;
  virtual void dispatch(std::shared_ptr<IRequest> req,
                        std::shared_ptr<IResponse> res) = 0;
};

} // namespace astra::router
