// =============================================================================
// test_log.cpp - Unit tests for obs::log functions
// =============================================================================
#include <gtest/gtest.h>
#include <obs/IBackend.h>
#include <obs/Log.h>
#include <obs/Context.h>
#include <obs/Span.h>
#include <vector>
#include <string>

namespace obs::test {

// -----------------------------------------------------------------------------
// MockBackend for logging tests
// -----------------------------------------------------------------------------
class LogMockBackend : public IBackend {
public:
    struct LogEntry {
        Level level;
        std::string message;
        Context ctx;
    };
    
    std::vector<LogEntry> logs;
    bool shutdown_called = false;
    
    void shutdown() override { shutdown_called = true; }
    
    std::unique_ptr<Span> create_span(std::string_view, const Context&) override { return nullptr; }
    std::unique_ptr<Span> create_root_span(std::string_view) override { return nullptr; }
    
    void log(Level level, std::string_view message, const Context& ctx) override {
        logs.push_back({level, std::string(message), ctx});
    }
    
    std::shared_ptr<Counter> get_counter(std::string_view, std::string_view) override { return nullptr; }
    std::shared_ptr<Histogram> get_histogram(std::string_view, std::string_view) override { return nullptr; }
    std::shared_ptr<Gauge> get_gauge(std::string_view, std::string_view) override { return nullptr; }
};

// -----------------------------------------------------------------------------
// Log Tests
// -----------------------------------------------------------------------------
class LogTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock = std::make_unique<LogMockBackend>();
        mock_ptr = mock.get();
        obs::set_backend(std::move(mock));
    }
    
    void TearDown() override {
        obs::shutdown();
    }
    
    std::unique_ptr<LogMockBackend> mock;
    LogMockBackend* mock_ptr;
};

TEST_F(LogTest, LogInfo) {
    Context ctx = Context::create();
    obs::info("test message", ctx);
    
    ASSERT_EQ(mock_ptr->logs.size(), 1);
    EXPECT_EQ(mock_ptr->logs[0].level, Level::INFO);
    EXPECT_EQ(mock_ptr->logs[0].message, "test message");
}

TEST_F(LogTest, LogError) {
    Context ctx = Context::create();
    obs::error("error message", ctx);
    
    ASSERT_EQ(mock_ptr->logs.size(), 1);
    EXPECT_EQ(mock_ptr->logs[0].level, Level::ERROR);
}

TEST_F(LogTest, LogWithoutContext) {
    obs::warn("warning");
    
    ASSERT_EQ(mock_ptr->logs.size(), 1);
    EXPECT_EQ(mock_ptr->logs[0].level, Level::WARN);
}

TEST_F(LogTest, LogPreservesContext) {
    Context ctx = Context::create();
    obs::debug("debug", ctx);
    
    ASSERT_EQ(mock_ptr->logs.size(), 1);
    EXPECT_EQ(mock_ptr->logs[0].ctx.trace_id.high, ctx.trace_id.high);
}

TEST_F(LogTest, AllLogLevels) {
    Context ctx = Context::create();
    obs::trace("t", ctx);
    obs::debug("d", ctx);
    obs::info("i", ctx);
    obs::warn("w", ctx);
    obs::error("e", ctx);
    obs::fatal("f", ctx);
    
    ASSERT_EQ(mock_ptr->logs.size(), 6);
    EXPECT_EQ(mock_ptr->logs[0].level, Level::TRACE);
    EXPECT_EQ(mock_ptr->logs[5].level, Level::FATAL);
}

} // namespace obs::test
