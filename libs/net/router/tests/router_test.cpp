#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Router.h"

using namespace testing;
using namespace router;

class RouterTest : public Test {
protected:
    Router m_router;
};

TEST_F(RouterTest, ExactMatch) {
    bool called = false;
    m_router.get("/users", [&](const IRequest&, IResponse&) {
        called = true;
    });
    
    auto result = m_router.match("GET", "/users");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_TRUE(result.params.empty());
}

TEST_F(RouterTest, ParamMatch) {
    m_router.get("/users/:id", [&](const IRequest&, IResponse&) {});
    
    auto result = m_router.match("GET", "/users/123");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_EQ(result.params.size(), 1);
    EXPECT_EQ(result.params.at("id"), "123");
}

TEST_F(RouterTest, NestedParams) {
    m_router.get("/users/:userId/posts/:postId", [&](const IRequest&, IResponse&) {});
    
    auto result = m_router.match("GET", "/users/123/posts/456");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_EQ(result.params.size(), 2);
    EXPECT_EQ(result.params.at("userId"), "123");
    EXPECT_EQ(result.params.at("postId"), "456");
}

TEST_F(RouterTest, CollisionPriority) {
    bool static_called = false;
    bool dynamic_called = false;
    
    m_router.get("/users/profile", [&](const IRequest&, IResponse&) { static_called = true; });
    m_router.get("/users/:id", [&](const IRequest&, IResponse&) { dynamic_called = true; });
    
    // Test Static Priority
    auto result_static = m_router.match("GET", "/users/profile");
    EXPECT_NE(result_static.handler, nullptr);
    EXPECT_TRUE(result_static.params.empty());
    
    // Test Dynamic Fallback
    auto result_dynamic = m_router.match("GET", "/users/123");
    EXPECT_NE(result_dynamic.handler, nullptr);
    EXPECT_EQ(result_dynamic.params.at("id"), "123");
}

TEST_F(RouterTest, NoMatch) {
    m_router.get("/users", [&](const IRequest&, IResponse&) {});
    
    auto result = m_router.match("GET", "/unknown");
    EXPECT_EQ(result.handler, nullptr);
    
    auto result_method = m_router.match("POST", "/users");
    EXPECT_EQ(result_method.handler, nullptr);
}
