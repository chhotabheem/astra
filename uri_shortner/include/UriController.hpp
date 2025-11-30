#pragma once

#include "IUriService.hpp"
#include "IRequest.hpp"
#include "IResponse.hpp"
#include <memory>

namespace uri_shortener {

class UriController final {
public:
    explicit UriController(std::shared_ptr<IUriService> service);
    ~UriController() = default;

    void shorten(router::IRequest& req, router::IResponse& res);
    void redirect(router::IRequest& req, router::IResponse& res);

private:
    std::shared_ptr<IUriService> service_;
};

} // namespace uri_shortener
