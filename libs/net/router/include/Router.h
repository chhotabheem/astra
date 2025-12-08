#pragma once

#include "Middleware.h"
#include <vector>
#include <string_view>
#include <string>
#include <unordered_map>
#include <memory>

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
    
    struct MatchResult {
        Handler handler;
        std::unordered_map<std::string, std::string> params;
    };
    
    [[nodiscard]] MatchResult match(std::string_view method, std::string_view path) const;
    void dispatch(router::IRequest& req, router::IResponse& res);

private:
    struct Node {
        std::unordered_map<std::string_view, std::unique_ptr<Node>> children;
        std::unique_ptr<Node> wildcard_child;
        std::string param_name;
        Handler handler;
    };

    std::unordered_map<std::string, std::unique_ptr<Node>> m_roots;
    std::vector<Middleware> m_middlewares;

    void add_route(std::string_view method, std::string_view path, Handler handler);
    void run_middleware(size_t index, router::IRequest& req, router::IResponse& res);
};

} // namespace router
