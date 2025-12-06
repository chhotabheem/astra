// =============================================================================
// test_console_backend.cpp - Tests for ConsoleBackend
// =============================================================================
// Verifies ConsoleBackend correctly outputs observability data to stderr.
// =============================================================================
#include <gtest/gtest.h>
#include <obs/Observability.h>
#include <sstream>
#include <iostream>

namespace obs::test {

class ConsoleBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Redirect stderr to capture output
        old_stderr = std::cerr.rdbuf();
        std::cerr.rdbuf(captured.rdbuf());
        
        obs::set_backend(std::make_unique<obs::ConsoleBackend>());
    }
    
    void TearDown() override {
        obs::shutdown();
        // Restore stderr
        std::cerr.rdbuf(old_stderr);
    }
    
    std::string getCapturedOutput() {
        std::cerr.flush();
        return captured.str();
    }
    
    std::stringstream captured;
    std::streambuf* old_stderr;
};

TEST_F(ConsoleBackendTest, InitializationOutput) {
    std::string output = getCapturedOutput();
    EXPECT_NE(output.find("[OBS] ConsoleBackend initialized"), std::string::npos)
        << "Output was: " << output;
}

TEST_F(ConsoleBackendTest, SpanStartEndOutput) {
    {
        auto span = obs::span("test_operation");
        ASSERT_NE(span, nullptr) << "span() should return non-null with ConsoleBackend";
    }
    
    std::string output = getCapturedOutput();
    EXPECT_NE(output.find("[SPAN START] test_operation"), std::string::npos)
        << "Output was: " << output;
    EXPECT_NE(output.find("[SPAN END] test_operation"), std::string::npos)
        << "Output was: " << output;
}

TEST_F(ConsoleBackendTest, SpanAttributeOutput) {
    auto span = obs::span("with_attrs");
    ASSERT_NE(span, nullptr) << "span() should return non-null with ConsoleBackend";
    
    // Now works with simple string literal thanks to const char* overload in Span.h
    span->attr("key", "value");
    span->attr("count", int64_t{42});
    
    std::string output = getCapturedOutput();
    EXPECT_NE(output.find("[SPAN ATTR] with_attrs key=value"), std::string::npos)
        << "Output was: " << output;
    EXPECT_NE(output.find("[SPAN ATTR] with_attrs count=42"), std::string::npos)
        << "Output was: " << output;
}

TEST_F(ConsoleBackendTest, SpanErrorOutput) {
    auto span = obs::span("failing_op");
    ASSERT_NE(span, nullptr);
    
    span->set_error("something went wrong");
    
    std::string output = getCapturedOutput();
    EXPECT_NE(output.find("[SPAN ERROR] failing_op something went wrong"), std::string::npos)
        << "Output was: " << output;
}

TEST_F(ConsoleBackendTest, LogOutput) {
    obs::info("Test info message");
    obs::warn("Test warning");
    obs::error("Test error");
    
    std::string output = getCapturedOutput();
    EXPECT_NE(output.find("[INFO] Test info message"), std::string::npos)
        << "Output was: " << output;
    EXPECT_NE(output.find("[WARN] Test warning"), std::string::npos)
        << "Output was: " << output;
    EXPECT_NE(output.find("[ERROR] Test error"), std::string::npos)
        << "Output was: " << output;
}

TEST_F(ConsoleBackendTest, LogWithContextIncludesTraceId) {
    auto ctx = obs::Context::create();
    obs::info("Traced message", ctx);
    
    std::string output = getCapturedOutput();
    EXPECT_NE(output.find("[INFO] Traced message trace="), std::string::npos)
        << "Output was: " << output;
}

TEST_F(ConsoleBackendTest, CounterOutput) {
    obs::counter("requests").inc();
    obs::counter("requests").inc(5);
    
    std::string output = getCapturedOutput();
    EXPECT_NE(output.find("[COUNTER] requests += 1"), std::string::npos)
        << "Output was: " << output;
    EXPECT_NE(output.find("[COUNTER] requests += 5"), std::string::npos)
        << "Output was: " << output;
}

TEST_F(ConsoleBackendTest, HistogramOutput) {
    obs::histogram("latency").record(0.042);
    
    std::string output = getCapturedOutput();
    EXPECT_NE(output.find("[HISTOGRAM] latency"), std::string::npos)
        << "Output was: " << output;
}

TEST_F(ConsoleBackendTest, ShutdownOutput) {
    obs::shutdown();
    
    std::string output = getCapturedOutput();
    EXPECT_NE(output.find("[OBS] ConsoleBackend shutdown"), std::string::npos)
        << "Output was: " << output;
    
    // Re-init for TearDown
    obs::set_backend(std::make_unique<obs::ConsoleBackend>());
}

TEST_F(ConsoleBackendTest, SpanIncludesTraceId) {
    auto span = obs::span("traced_span");
    ASSERT_NE(span, nullptr);
    
    std::string output = getCapturedOutput();
    EXPECT_NE(output.find("trace="), std::string::npos)
        << "Output was: " << output;
}

} // namespace obs::test
