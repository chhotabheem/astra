#include "Router.h"
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>

namespace astra::router {

Router::Router() = default;
Router::~Router() = default;

void Router::get(const std::string& path, Handler handler) {
    add_route("GET", path, std::move(handler));
}

void Router::post(const std::string& path, Handler handler) {
    add_route("POST", path, std::move(handler));
}

void Router::put(const std::string& path, Handler handler) {
    add_route("PUT", path, std::move(handler));
}

void Router::del(const std::string& path, Handler handler) {
    add_route("DELETE", path, std::move(handler));
}

// Helper to split path into segments
std::vector<std::string> split_path(const std::string& path) {
    std::vector<std::string> segments;
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

void Router::add_route(const std::string& method, const std::string& path, Handler handler) {
    if (m_roots.find(method) == m_roots.end()) {
        m_roots[method] = std::make_unique<Node>();
    }
    
    Node* current = m_roots[method].get();
    auto segments = split_path(path);
    
    for (const auto& segment : segments) {
        if (segment.empty()) continue;
        
        if (segment[0] == ':') {
            // Wildcard / Parameter
            if (!current->wildcard_child) {
                current->wildcard_child = std::make_unique<Node>();
                current->wildcard_child->param_name = segment.substr(1);
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

Router::MatchResult Router::match(const std::string& method, const std::string& path) const {
    auto it = m_roots.find(method);
    if (it == m_roots.end()) {
        return {nullptr, {}};
    }
    
    Node* current = it->second.get();
    std::unordered_map<std::string, std::string> params;
    auto segments = split_path(path);
    
    for (const auto& segment : segments) {
        if (segment.empty()) continue;
        
        auto child_it = current->children.find(segment);
        if (child_it != current->children.end()) {
            current = child_it->second.get();
        } else if (current->wildcard_child) {
            params[current->wildcard_child->param_name] = segment;
            current = current->wildcard_child.get();
        } else {
            return {nullptr, {}};
        }
    }
    
    if (!current->handler) {
        return {nullptr, {}};
    }
    
    return {current->handler, std::move(params)};
}

void Router::dispatch(std::shared_ptr<IRequest> req, std::shared_ptr<IResponse> res) {
    auto result = match(req->method(), req->path());
    
    if (result.handler) {
        req->set_path_params(std::move(result.params));
        result.handler(req, res);
    } else {
        res->set_status(404);
        res->write("Not Found");
        res->close();
    }
}

} // namespace astra::router
