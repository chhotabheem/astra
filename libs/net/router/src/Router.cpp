#include "Router.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>

namespace router {

Router::Router() = default;
Router::~Router() = default;

void Router::use(Middleware middleware) {
    middlewares_.push_back(std::move(middleware));
}

void Router::get(std::string_view path, Handler handler) {
    add_route("GET", path, std::move(handler));
}

void Router::post(std::string_view path, Handler handler) {
    add_route("POST", path, std::move(handler));
}

void Router::put(std::string_view path, Handler handler) {
    add_route("PUT", path, std::move(handler));
}

void Router::del(std::string_view path, Handler handler) {
    add_route("DELETE", path, std::move(handler));
}

// Helper to split path into segments
std::vector<std::string_view> split_path(std::string_view path) {
    std::vector<std::string_view> segments;
    size_t start = 0;
    if (!path.empty() && path[0] == '/') start = 1; // Skip leading slash
    
    for (size_t i = start; i < path.length(); ++i) {
        if (path[i] == '/') {
            if (i > start) segments.push_back(path.substr(start, i - start));
            start = i + 1;
        }
    }
    if (start < path.length()) {
        segments.push_back(path.substr(start));
    }
    return segments;
}

void Router::add_route(std::string_view method, std::string_view path, Handler handler) {
    std::string method_str(method);
    if (roots_.find(method_str) == roots_.end()) {
        roots_[method_str] = std::make_unique<Node>();
    }
    
    Node* current = roots_[method_str].get();
    auto segments = split_path(path);
    
    for (const auto& segment : segments) {
        if (segment.empty()) continue;
        
        if (segment[0] == ':') {
            // Wildcard / Parameter
            if (!current->wildcard_child) {
                current->wildcard_child = std::make_unique<Node>();
                current->wildcard_child->param_name = segment.substr(1); // Store "id" from ":id"
            }
            current = current->wildcard_child.get();
        } else {
            // Static Segment
            if (current->children.find(segment) == current->children.end()) {
                current->children[segment] = std::make_unique<Node>();
            }
            current = current->children[segment].get();
        }
    }
    
    current->handler = std::move(handler);
}

Router::MatchResult Router::match(std::string_view method, std::string_view path) const {
    std::string method_str(method);
    auto it = roots_.find(method_str);
    if (it == roots_.end()) {
        return {nullptr, {}};
    }
    
    Node* current = it->second.get();
    std::unordered_map<std::string_view, std::string_view> params;
    auto segments = split_path(path);
    
    for (const auto& segment : segments) {
        if (segment.empty()) continue;
        
        // Priority: Static Match > Wildcard Match
        auto child_it = current->children.find(segment);
        if (child_it != current->children.end()) {
            current = child_it->second.get();
        } else if (current->wildcard_child) {
            params[current->wildcard_child->param_name] = segment;
            current = current->wildcard_child.get();
        } else {
            return {nullptr, {}}; // No match
        }
    }
    
    if (!current->handler) {
        return {nullptr, {}}; // Path matched but no handler at this leaf
    }
    
    return {current->handler, std::move(params)};
}

void Router::dispatch(router::IRequest& req, router::IResponse& res) {
    auto result = match(req.method(), req.path());
    
    if (result.handler) {
        req.set_path_params(std::move(result.params));
        run_middleware(0, req, res);
    } else {
        res.set_status(404);
        res.write("Not Found");
        res.close();
    }
}

void Router::run_middleware(size_t index, router::IRequest& req, router::IResponse& res) {
    if (index < middlewares_.size()) {
        middlewares_[index](req, res, [this, index, &req, &res]() {
            run_middleware(index + 1, req, res);
        });
    } else {
        // Final Handler Execution
        // We need to re-match here because dispatch() is stateless regarding the match result.
        // This is inefficient. Ideally, dispatch should take the MatchResult.
        // But since we injected params into req, the handler can access them.
        auto result = match(req.method(), req.path());
        if (result.handler) {
            result.handler(req, res);
        }
    }
}

} // namespace router
