#include "router/Router.hpp"
#include <iostream>

namespace router {

Router::Router() = default;
Router::~Router() = default;

void Router::use(Middleware middleware) {
    middlewares_.push_back(std::move(middleware));
}

void Router::get(std::string_view path, Handler handler) {
    handle("GET", path, std::move(handler));
}

void Router::post(std::string_view path, Handler handler) {
    handle("POST", path, std::move(handler));
}

void Router::put(std::string_view path, Handler handler) {
    handle("PUT", path, std::move(handler));
}

void Router::del(std::string_view path, Handler handler) {
    handle("DELETE", path, std::move(handler));
}

void Router::handle(std::string_view method, std::string_view path, Handler handler) {
    routes_.push_back({std::string(method), std::string(path), std::move(handler)});
}

void Router::dispatch(const http_abstractions::IRequest& req, http_abstractions::IResponse& res) {
    run_middleware(0, req, res);
}

void Router::run_middleware(size_t index, const http_abstractions::IRequest& req, http_abstractions::IResponse& res) {
    if (index < middlewares_.size()) {
        middlewares_[index](req, res, [this, index, &req, &res]() {
            run_middleware(index + 1, req, res);
        });
    } else {
        // No more middleware, find route
        bool found = false;
        for (const auto& route : routes_) {
            if (route.method == req.method() && route.path == req.path()) {
                route.handler(req, res);
                found = true;
                break;
            }
        }
        
        if (!found) {
            res.set_status(404);
            res.write("Not Found");
            res.close();
        }
    }
}

} // namespace router
