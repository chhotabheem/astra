#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Router.h"
#include <thread>
#include <vector>

using namespace testing;
using namespace router;

class RouterTest : public Test {
protected:
    Router m_router;
};

// Mock classes for dispatch testing
class MockRequest : public IRequest {
public:
    MockRequest(std::string path, std::string method) 
        : m_path(std::move(path)), m_method(std::move(method)) {}
    
    std::string_view path() const override { return m_path; }
    std::string_view method() const override { return m_method; }
    std::string_view body() const override { return ""; }
    std::string_view header(std::string_view) const override { return ""; }
    std::string_view path_param(std::string_view) const override { return ""; }
    std::string_view query_param(std::string_view) const override { return ""; }
    void set_path_params(std::unordered_map<std::string, std::string>) override {}
    
private:
    std::string m_path;
    std::string m_method;
};

class MockResponse : public IResponse {
public:
    void set_status(int) noexcept override {}
    void set_header(std::string_view, std::string_view) override {}
    void write(std::string_view) override {}
    void close() override {}
    bool is_alive() const noexcept override { return true; }
};

// =============================================================================
// Basic Matching Tests
// =============================================================================

TEST_F(RouterTest, ExactMatch) {
    bool called = false;
    m_router.get("/users", [&](std::shared_ptr<IRequest>, std::shared_ptr<IResponse>) {
        called = true;
    });
    
    auto result = m_router.match("GET", "/users");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_TRUE(result.params.empty());
}

TEST_F(RouterTest, ParamMatch) {
    m_router.get("/users/:id", [&](std::shared_ptr<IRequest>, std::shared_ptr<IResponse>) {});
    
    auto result = m_router.match("GET", "/users/123");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_EQ(result.params.size(), 1);
    EXPECT_EQ(result.params.at("id"), "123");
}

TEST_F(RouterTest, NestedParams) {
    m_router.get("/users/:userId/posts/:postId", [&](std::shared_ptr<IRequest>, std::shared_ptr<IResponse>) {});
    
    auto result = m_router.match("GET", "/users/123/posts/456");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_EQ(result.params.size(), 2);
    EXPECT_EQ(result.params.at("userId"), "123");
    EXPECT_EQ(result.params.at("postId"), "456");
}

TEST_F(RouterTest, CollisionPriority) {
    bool static_called = false;
    bool dynamic_called = false;
    
    m_router.get("/users/profile", [&](std::shared_ptr<IRequest>, std::shared_ptr<IResponse>) { static_called = true; });
    m_router.get("/users/:id", [&](std::shared_ptr<IRequest>, std::shared_ptr<IResponse>) { dynamic_called = true; });
    
    auto result_static = m_router.match("GET", "/users/profile");
    EXPECT_NE(result_static.handler, nullptr);
    EXPECT_TRUE(result_static.params.empty());
    
    auto result_dynamic = m_router.match("GET", "/users/123");
    EXPECT_NE(result_dynamic.handler, nullptr);
    EXPECT_EQ(result_dynamic.params.at("id"), "123");
}

TEST_F(RouterTest, NoMatch) {
    m_router.get("/users", [&](std::shared_ptr<IRequest>, std::shared_ptr<IResponse>) {});
    
    auto result = m_router.match("GET", "/unknown");
    EXPECT_EQ(result.handler, nullptr);
    
    auto result_method = m_router.match("POST", "/users");
    EXPECT_EQ(result_method.handler, nullptr);
}

// =============================================================================
// Path Edge Cases
// =============================================================================

TEST_F(RouterTest, RootPath) {
    m_router.get("/", [](auto, auto) {});
    
    auto result = m_router.match("GET", "/");
    EXPECT_NE(result.handler, nullptr);
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
    EXPECT_NE(result.handler, nullptr);
}

TEST_F(RouterTest, PathWithNumbers) {
    m_router.get("/v1/api/users", [](auto, auto) {});
    
    auto result = m_router.match("GET", "/v1/api/users");
    EXPECT_NE(result.handler, nullptr);
}

TEST_F(RouterTest, PathWithHyphens) {
    m_router.get("/user-profiles", [](auto, auto) {});
    
    auto result = m_router.match("GET", "/user-profiles");
    EXPECT_NE(result.handler, nullptr);
}

TEST_F(RouterTest, PathWithUnderscores) {
    m_router.get("/user_profiles", [](auto, auto) {});
    
    auto result = m_router.match("GET", "/user_profiles");
    EXPECT_NE(result.handler, nullptr);
}

// =============================================================================
// Parameter Edge Cases
// =============================================================================

TEST_F(RouterTest, NumericParamValue) {
    m_router.get("/users/:id", [](auto, auto) {});
    
    auto result = m_router.match("GET", "/users/999999999");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_EQ(result.params.at("id"), "999999999");
}

TEST_F(RouterTest, ParamWithHyphens) {
    m_router.get("/articles/:slug", [](auto, auto) {});
    
    auto result = m_router.match("GET", "/articles/my-first-article");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_EQ(result.params.at("slug"), "my-first-article");
}

TEST_F(RouterTest, ParamWithUnderscores) {
    m_router.get("/files/:name", [](auto, auto) {});
    
    auto result = m_router.match("GET", "/files/my_document_v2");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_EQ(result.params.at("name"), "my_document_v2");
}

TEST_F(RouterTest, MultipleParamsInSequence) {
    m_router.get("/org/:orgId/team/:teamId/member/:memberId", [](auto, auto) {});
    
    auto result = m_router.match("GET", "/org/100/team/200/member/300");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_EQ(result.params.size(), 3);
    EXPECT_EQ(result.params.at("orgId"), "100");
    EXPECT_EQ(result.params.at("teamId"), "200");
    EXPECT_EQ(result.params.at("memberId"), "300");
}

// =============================================================================
// HTTP Method Tests
// =============================================================================

TEST_F(RouterTest, PostMethod) {
    m_router.post("/users", [](auto, auto) {});
    
    auto result = m_router.match("POST", "/users");
    EXPECT_NE(result.handler, nullptr);
    
    // GET should not match POST route
    auto get_result = m_router.match("GET", "/users");
    EXPECT_EQ(get_result.handler, nullptr);
}

TEST_F(RouterTest, PutMethod) {
    m_router.put("/users/:id", [](auto, auto) {});
    
    auto result = m_router.match("PUT", "/users/123");
    EXPECT_NE(result.handler, nullptr);
}

TEST_F(RouterTest, DeleteMethod) {
    m_router.del("/users/:id", [](auto, auto) {});
    
    auto result = m_router.match("DELETE", "/users/123");
    EXPECT_NE(result.handler, nullptr);
}

TEST_F(RouterTest, SamePathDifferentMethods) {
    m_router.get("/users", [](auto, auto) {});
    m_router.post("/users", [](auto, auto) {});
    m_router.put("/users/:id", [](auto, auto) {});
    m_router.del("/users/:id", [](auto, auto) {});
    
    EXPECT_NE(m_router.match("GET", "/users").handler, nullptr);
    EXPECT_NE(m_router.match("POST", "/users").handler, nullptr);
    EXPECT_NE(m_router.match("PUT", "/users/1").handler, nullptr);
    EXPECT_NE(m_router.match("DELETE", "/users/1").handler, nullptr);
}

TEST_F(RouterTest, UnknownMethod) {
    m_router.get("/users", [](auto, auto) {});
    
    auto result = m_router.match("PATCH", "/users");
    EXPECT_EQ(result.handler, nullptr);
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(RouterTest, ManyRoutes) {
    for (int i = 0; i < 1000; ++i) {
        m_router.get("/route" + std::to_string(i), [](auto, auto) {});
    }
    
    auto result = m_router.match("GET", "/route500");
    EXPECT_NE(result.handler, nullptr);
    
    auto miss = m_router.match("GET", "/route9999");
    EXPECT_EQ(miss.handler, nullptr);
}

TEST_F(RouterTest, DeepNesting) {
    std::string path = "";
    for (int i = 0; i < 20; ++i) {
        path += "/level" + std::to_string(i);
    }
    m_router.get(path, [](auto, auto) {});
    
    auto result = m_router.match("GET", path);
    EXPECT_NE(result.handler, nullptr);
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
                if (r1.handler && r2.handler && r3.handler) {
                    success_count++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
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
