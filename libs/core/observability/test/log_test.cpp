#include <Log.h>
#include <Span.h>
#include <Provider.h>
#include <gtest/gtest.h>

class LogTest : public ::testing::Test {
protected:
    void SetUp() override {
        obs::Config config{.service_name = "test-service"};
        obs::init(config);
    }
    
    void TearDown() override {
        obs::shutdown();
    }
};

TEST_F(LogTest, BasicLogging) {
    EXPECT_NO_THROW(obs::trace("trace message"));
    EXPECT_NO_THROW(obs::debug("debug message"));
    EXPECT_NO_THROW(obs::info("info message"));
    EXPECT_NO_THROW(obs::warn("warn message"));
    EXPECT_NO_THROW(obs::error("error message"));
    EXPECT_NO_THROW(obs::fatal("fatal message"));
}

TEST_F(LogTest, LogWithAttributes) {
    EXPECT_NO_THROW(obs::info("test", {
        {"key1", "value1"},
        {"key2", "value2"}
    }));
    
    EXPECT_NO_THROW(obs::error("error test", {
        {"error.type", "test"},
        {"error.message", "test error"}
    }));
}

TEST_F(LogTest, ScopedAttributes) {
    {
        obs::ScopedLogAttributes scoped({
            {"request.id", "req-123"},
            {"session.id", "sess-456"}
        });
        
        // Logs in this scope should inherit scoped attributes
        EXPECT_NO_THROW(obs::info("message with scoped attrs"));
    }
    
    // Scoped attributes should be removed now
    EXPECT_NO_THROW(obs::info("message without scoped attrs"));
}

TEST_F(LogTest, AutomaticTraceCorrelation) {
    {
        auto span = obs::span("test.operation");
        auto ctx = span.context();
        
        // Log within span should automatically include trace_id and span_id
        EXPECT_NO_THROW(obs::info("log with trace correlation"));
        
        // Note: Actual verification would require capturing log output
        // and checking for trace_id/span_id attributes
    }
}

TEST_F(LogTest, NestedScopedAttributes) {
    {
        obs::ScopedLogAttributes scope1({{"key1", "value1"}});
        
        {
            obs::ScopedLogAttributes scope2({{"key2", "value2"}});
            
            // Should have both scope1 and scope2 attributes
            EXPECT_NO_THROW(obs::info("nested scoped log"));
        }
        
        // Should only have scope1 attributes
        EXPECT_NO_THROW(obs::info("outer scoped log"));
    }
}

TEST_F(LogTest, LogWithinSpanAndScope) {
    {
        auto span = obs::span("request");
        span.attr("request.id", "req-123");
        
        obs::ScopedLogAttributes scoped({
            {"user.id", "user-456"}
        });
        
        // Log should have:
        // - trace_id, span_id (from span)
        // - user.id (from scoped)
        EXPECT_NO_THROW(obs::info("complete log"));
    }
}

TEST_F(LogTest, AllLogLevels) {
    EXPECT_NO_THROW(obs::log(obs::Level::Trace, "trace"));
    EXPECT_NO_THROW(obs::log(obs::Level::Debug, "debug"));
    EXPECT_NO_THROW(obs::log(obs::Level::Info, "info"));
    EXPECT_NO_THROW(obs::log(obs::Level::Warn, "warn"));
    EXPECT_NO_THROW(obs::log(obs::Level::Error, "error"));
    EXPECT_NO_THROW(obs::log(obs::Level::Fatal, "fatal"));
}
