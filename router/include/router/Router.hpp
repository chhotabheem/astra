#pragma once

#include "Middleware.hpp"
#include <vector>
#include <string_view>
#include <string>

namespace router {

class Router {
public:
    Router();
    ~Router();

    void use(Middleware middleware);
    
    void get(std::string_view path, Handler handler);
    void post(std::string_view path, Handler handler);
    void put(std::string_view path, Handler handler);
    void del(std::string_view path, Handler handler);
    
    void handle(std::string_view method, std::string_view path, Handler handler);
    
    void dispatch(const http_abstractions::IRequest& req, http_abstractions::IResponse& res);

private:
    struct Route {
        std::string method;
        std::string path;
        Handler handler;
    };

    std::vector<Middleware> middlewares_;
    std::vector<Route> routes_;
    
    void run_middleware(size_t index, const http_abstractions::IRequest& req, http_abstractions::IResponse& res);
};

} // namespace router
