#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Router.h"

using namespace testing;
using namespace router;

class RouterTest : public Test {
protected:
    Router router_;
};

TEST_F(RouterTest, ExactMatch) {
    bool called = false;
    router_.get("/users", [&](const IRequest&, IResponse&) {
        called = true;
    });
    
    auto result = router_.match("GET", "/users");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_TRUE(result.params.empty());
}

TEST_F(RouterTest, ParamMatch) {
    router_.get("/users/:id", [&](const IRequest&, IResponse&) {});
    
    auto result = router_.match("GET", "/users/123");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_EQ(result.params.size(), 1);
    EXPECT_EQ(result.params.at("id"), "123");
}

TEST_F(RouterTest, NestedParams) {
    router_.get("/users/:userId/posts/:postId", [&](const IRequest&, IResponse&) {});
    
    auto result = router_.match("GET", "/users/123/posts/456");
    EXPECT_NE(result.handler, nullptr);
    EXPECT_EQ(result.params.size(), 2);
    EXPECT_EQ(result.params.at("userId"), "123");
    EXPECT_EQ(result.params.at("postId"), "456");
}

TEST_F(RouterTest, CollisionPriority) {
    bool static_called = false;
    bool dynamic_called = false;
    
    router_.get("/users/profile", [&](const IRequest&, IResponse&) { static_called = true; });
    router_.get("/users/:id", [&](const IRequest&, IResponse&) { dynamic_called = true; });
    
    // Test Static Priority
    auto result_static = router_.match("GET", "/users/profile");
    EXPECT_NE(result_static.handler, nullptr);
    EXPECT_TRUE(result_static.params.empty());
    
    // Test Dynamic Fallback
    auto result_dynamic = router_.match("GET", "/users/123");
    EXPECT_NE(result_dynamic.handler, nullptr);
    EXPECT_EQ(result_dynamic.params.at("id"), "123");
}

TEST_F(RouterTest, NoMatch) {
    router_.get("/users", [&](const IRequest&, IResponse&) {});
    
    auto result = router_.match("GET", "/unknown");
    EXPECT_EQ(result.handler, nullptr);
    
    auto result_method = router_.match("POST", "/users");
    EXPECT_EQ(result_method.handler, nullptr);
}
