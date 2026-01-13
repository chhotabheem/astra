#pragma once

#include "IRouter.h"

#include <optional>
#include <unordered_map>
#include <vector>

namespace astra::router {

class Router : public IRouter {
public:
  Router() = default;
  ~Router() override = default;

  void add(HttpMethod method, const std::string &path,
           Handler handler) override;
  void dispatch(std::shared_ptr<IRequest> req,
                std::shared_ptr<IResponse> res) override;

  struct MatchResult {
    Handler handler;
    std::unordered_map<std::string, std::string> params;
  };

  [[nodiscard]] std::optional<MatchResult> match(const std::string &method,
                                                 const std::string &path) const;

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
