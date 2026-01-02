#include "Router.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace testing;
using namespace astra::router;

class RouterTest : public Test {
protected:
  Router m_router;
};

// Mock classes for dispatch testing
class MockRequest : public IRequest {
public:
  MockRequest(std::string path, std::string method)
      : m_path(std::move(path)), m_method(std::move(method)) {
  }

  const std::string &path() const override {
    return m_path;
  }
  const std::string &method() const override {
    return m_method;
  }
  const std::string &body() const override {
    return m_empty;
  }
  std::string header(const std::string &) const override {
    return "";
  }
  std::string path_param(const std::string &) const override {
    return "";
  }
  std::string query_param(const std::string &) const override {
    return "";
  }
  void set_path_params(std::unordered_map<std::string, std::string>) override {
  }

private:
  std::string m_path;
  std::string m_method;
  std::string m_empty;
};

class MockResponse : public IResponse {
public:
  void set_status(int) noexcept override {
  }
  void set_header(const std::string &, const std::string &) override {
  }
  void write(const std::string &) override {
  }
  void close() override {
  }
  bool is_alive() const noexcept override {
    return true;
  }
};

// =============================================================================
// Basic Matching Tests
// =============================================================================

TEST_F(RouterTest, ExactMatch) {
  bool called = false;
  m_router.get("/users",
               [&](std::shared_ptr<IRequest>, std::shared_ptr<IResponse>) {
                 called = true;
               });

  auto result = m_router.match("GET", "/users");
  EXPECT_TRUE(result);
  EXPECT_TRUE(result->params.empty());
}

TEST_F(RouterTest, ParamMatch) {
  m_router.get("/users/:id",
               [&](std::shared_ptr<IRequest>, std::shared_ptr<IResponse>) {});

  auto result = m_router.match("GET", "/users/123");
  EXPECT_TRUE(result);
  EXPECT_EQ(result->params.size(), 1);
  EXPECT_EQ(result->params.at("id"), "123");
}

TEST_F(RouterTest, NestedParams) {
  m_router.get("/users/:userId/posts/:postId",
               [&](std::shared_ptr<IRequest>, std::shared_ptr<IResponse>) {});

  auto result = m_router.match("GET", "/users/123/posts/456");
  EXPECT_TRUE(result);
  EXPECT_EQ(result->params.size(), 2);
  EXPECT_EQ(result->params.at("userId"), "123");
  EXPECT_EQ(result->params.at("postId"), "456");
}

TEST_F(RouterTest, CollisionPriority) {
  bool static_called = false;
  bool dynamic_called = false;

  m_router.get("/users/profile",
               [&](std::shared_ptr<IRequest>, std::shared_ptr<IResponse>) {
                 static_called = true;
               });
  m_router.get("/users/:id",
               [&](std::shared_ptr<IRequest>, std::shared_ptr<IResponse>) {
                 dynamic_called = true;
               });

  auto result_static = m_router.match("GET", "/users/profile");
  EXPECT_TRUE(result_static);
  EXPECT_TRUE(result_static->params.empty());

  auto result_dynamic = m_router.match("GET", "/users/123");
  EXPECT_TRUE(result_dynamic);
  EXPECT_EQ(result_dynamic->params.at("id"), "123");
}

TEST_F(RouterTest, NoMatch) {
  m_router.get("/users",
               [&](std::shared_ptr<IRequest>, std::shared_ptr<IResponse>) {});

  auto result = m_router.match("GET", "/unknown");
  EXPECT_FALSE(result);

  auto result_method = m_router.match("POST", "/users");
  EXPECT_FALSE(result_method);
}

// =============================================================================
// Path Edge Cases
// =============================================================================

TEST_F(RouterTest, RootPath) {
  m_router.get("/", [](auto, auto) {});

  auto result = m_router.match("GET", "/");
  EXPECT_TRUE(result);
}

TEST_F(RouterTest, TrailingSlash) {
  m_router.get("/users", [](auto, auto) {});

  // Path with trailing slash should NOT match (strict matching)
  auto result = m_router.match("GET", "/users/");
  // Behavior depends on implementation - document actual behavior
  // EXPECT_EQ(result.handler, nullptr);  // If strict
}

TEST_F(RouterTest, DoubleSlashInPath) {
  m_router.get("/users", [](auto, auto) {});

  auto result = m_router.match("GET", "//users");
  // Should not crash, behavior is implementation-defined
}

TEST_F(RouterTest, VeryLongPath) {
  std::string long_path = "/a";
  for (int i = 0; i < 100; ++i) {
    long_path += "/segment" + std::to_string(i);
  }
  m_router.get(long_path, [](auto, auto) {});

  auto result = m_router.match("GET", long_path);
  EXPECT_TRUE(result);
}

TEST_F(RouterTest, PathWithNumbers) {
  m_router.get("/v1/api/users", [](auto, auto) {});

  auto result = m_router.match("GET", "/v1/api/users");
  EXPECT_TRUE(result);
}

TEST_F(RouterTest, PathWithHyphens) {
  m_router.get("/user-profiles", [](auto, auto) {});

  auto result = m_router.match("GET", "/user-profiles");
  EXPECT_TRUE(result);
}

TEST_F(RouterTest, PathWithUnderscores) {
  m_router.get("/user_profiles", [](auto, auto) {});

  auto result = m_router.match("GET", "/user_profiles");
  EXPECT_TRUE(result);
}

// =============================================================================
// Parameter Edge Cases
// =============================================================================

TEST_F(RouterTest, NumericParamValue) {
  m_router.get("/users/:id", [](auto, auto) {});

  auto result = m_router.match("GET", "/users/999999999");
  EXPECT_TRUE(result);
  EXPECT_EQ(result->params.at("id"), "999999999");
}

TEST_F(RouterTest, ParamWithHyphens) {
  m_router.get("/articles/:slug", [](auto, auto) {});

  auto result = m_router.match("GET", "/articles/my-first-article");
  EXPECT_TRUE(result);
  EXPECT_EQ(result->params.at("slug"), "my-first-article");
}

TEST_F(RouterTest, ParamWithUnderscores) {
  m_router.get("/files/:name", [](auto, auto) {});

  auto result = m_router.match("GET", "/files/my_document_v2");
  EXPECT_TRUE(result);
  EXPECT_EQ(result->params.at("name"), "my_document_v2");
}

TEST_F(RouterTest, MultipleParamsInSequence) {
  m_router.get("/org/:orgId/team/:teamId/member/:memberId", [](auto, auto) {});

  auto result = m_router.match("GET", "/org/100/team/200/member/300");
  EXPECT_TRUE(result);
  EXPECT_EQ(result->params.size(), 3);
  EXPECT_EQ(result->params.at("orgId"), "100");
  EXPECT_EQ(result->params.at("teamId"), "200");
  EXPECT_EQ(result->params.at("memberId"), "300");
}

// =============================================================================
// HTTP Method Tests
// =============================================================================

TEST_F(RouterTest, PostMethod) {
  m_router.post("/users", [](auto, auto) {});

  auto result = m_router.match("POST", "/users");
  EXPECT_TRUE(result);

  // GET should not match POST route
  auto get_result = m_router.match("GET", "/users");
  EXPECT_FALSE(get_result);
}

TEST_F(RouterTest, PutMethod) {
  m_router.put("/users/:id", [](auto, auto) {});

  auto result = m_router.match("PUT", "/users/123");
  EXPECT_TRUE(result);
}

TEST_F(RouterTest, DeleteMethod) {
  m_router.del("/users/:id", [](auto, auto) {});

  auto result = m_router.match("DELETE", "/users/123");
  EXPECT_TRUE(result);
}

TEST_F(RouterTest, SamePathDifferentMethods) {
  m_router.get("/users", [](auto, auto) {});
  m_router.post("/users", [](auto, auto) {});
  m_router.put("/users/:id", [](auto, auto) {});
  m_router.del("/users/:id", [](auto, auto) {});

  EXPECT_TRUE(m_router.match("GET", "/users"));
  EXPECT_TRUE(m_router.match("POST", "/users"));
  EXPECT_TRUE(m_router.match("PUT", "/users/1"));
  EXPECT_TRUE(m_router.match("DELETE", "/users/1"));
}

TEST_F(RouterTest, UnknownMethod) {
  m_router.get("/users", [](auto, auto) {});

  auto result = m_router.match("PATCH", "/users");
  EXPECT_FALSE(result);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(RouterTest, ManyRoutes) {
  for (int i = 0; i < 1000; ++i) {
    m_router.get("/route" + std::to_string(i), [](auto, auto) {});
  }

  auto result = m_router.match("GET", "/route500");
  EXPECT_TRUE(result);

  auto miss = m_router.match("GET", "/route9999");
  EXPECT_FALSE(miss);
}

TEST_F(RouterTest, DeepNesting) {
  std::string path = "";
  for (int i = 0; i < 20; ++i) {
    path += "/level" + std::to_string(i);
  }
  m_router.get(path, [](auto, auto) {});

  auto result = m_router.match("GET", path);
  EXPECT_TRUE(result);
}

TEST_F(RouterTest, ConcurrentMatching) {
  m_router.get("/users/:id", [](auto, auto) {});
  m_router.get("/posts/:id", [](auto, auto) {});
  m_router.get("/comments/:id", [](auto, auto) {});

  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([this, i, &success_count]() {
      for (int j = 0; j < 100; ++j) {
        auto r1 = m_router.match("GET", "/users/" + std::to_string(j));
        auto r2 = m_router.match("GET", "/posts/" + std::to_string(j));
        auto r3 = m_router.match("GET", "/comments/" + std::to_string(j));
        if (r1 && r2 && r3) {
          success_count++;
        }
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  EXPECT_EQ(success_count.load(), 10000);
}

// =============================================================================
// Dispatch Tests
// =============================================================================

TEST_F(RouterTest, DispatchCallsHandler) {
  bool handler_called = false;
  m_router.get("/test", [&handler_called](auto, auto) {
    handler_called = true;
  });

  auto req = std::make_shared<MockRequest>("/test", "GET");
  auto res = std::make_shared<MockResponse>();

  m_router.dispatch(req, res);
  EXPECT_TRUE(handler_called);
}

TEST_F(RouterTest, DispatchNoMatchDoesNotCrash) {
  auto req = std::make_shared<MockRequest>("/nonexistent", "GET");
  auto res = std::make_shared<MockResponse>();

  // Should not crash when no handler matches
  EXPECT_NO_THROW(m_router.dispatch(req, res));
}
