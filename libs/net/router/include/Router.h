#pragma once

#include "IRequest.h"
#include "IResponse.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace astra::router {

using Handler =
    std::function<void(std::shared_ptr<IRequest>, std::shared_ptr<IResponse>)>;

class Router {
public:
  Router() = default;
  ~Router() = default;

  void get(const std::string &path, Handler handler);
  void post(const std::string &path, Handler handler);
  void put(const std::string &path, Handler handler);
  void del(const std::string &path, Handler handler);

  struct MatchResult {
    Handler handler;
    std::unordered_map<std::string, std::string> params;
  };

  [[nodiscard]] std::optional<MatchResult> match(const std::string &method,
                                                 const std::string &path) const;
  void dispatch(std::shared_ptr<IRequest> req, std::shared_ptr<IResponse> res);

private:
  struct Node {
    std::unordered_map<std::string, std::unique_ptr<Node>> children;
    std::unique_ptr<Node> wildcard_child;
    std::string param_name;
    Handler handler;
  };

  std::unordered_map<std::string, std::unique_ptr<Node>> m_roots;

  void add_route(const std::string &method, const std::string &path,
                 Handler handler);
};

} // namespace astra::router
