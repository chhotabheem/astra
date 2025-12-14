#pragma once

#include "IRequest.h"
#include "IResponse.h"
#include <vector>
#include <string_view>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

namespace astra::router {

/// Handler takes shared_ptr to request and response
using Handler = std::function<void(std::shared_ptr<IRequest>, std::shared_ptr<IResponse>)>;

class Router {
public:
    Router();
    ~Router();
    
    void get(std::string_view path, Handler handler);
    void post(std::string_view path, Handler handler);
    void put(std::string_view path, Handler handler);
    void del(std::string_view path, Handler handler);
    
    struct MatchResult {
        Handler handler;
        std::unordered_map<std::string, std::string> params;
    };
    
    [[nodiscard]] MatchResult match(std::string_view method, std::string_view path) const;
    void dispatch(std::shared_ptr<IRequest> req, std::shared_ptr<IResponse> res);

private:
    struct Node {
        std::unordered_map<std::string_view, std::unique_ptr<Node>> children;
        std::unique_ptr<Node> wildcard_child;
        std::string param_name;
        Handler handler;
    };

    std::unordered_map<std::string, std::unique_ptr<Node>> m_roots;

    void add_route(std::string_view method, std::string_view path, Handler handler);
};

} // namespace astra::router

// Backward compatibility alias
namespace router = astra::router;
