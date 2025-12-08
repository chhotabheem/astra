// =============================================================================
// test_seda_tracing.cpp - End-to-end SEDA context propagation tests
// =============================================================================
// Simulates real SEDA flow: Network -> Worker -> IO with trace context.
// =============================================================================
#include <gtest/gtest.h>
#include <obs/Observability.h>
#include <Job.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace obs::test {

using astra::execution::Job;
using astra::execution::JobType;

// Helper to create Job (C++17 compatible)
inline Job make_job(JobType type, uint64_t session_id, std::any payload, obs::Context ctx) {
    Job job;
    job.type = type;
    job.session_id = session_id;
    job.payload = std::move(payload);
    job.trace_ctx = ctx;
    return job;
}

// =============================================================================
// Simulated SEDA Stage (simplified)
// =============================================================================
class MockStage {
public:
    void submit(Job job) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(job));
        m_cv.notify_one();
    }
    
    Job take() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_queue.empty(); });
        Job job = std::move(m_queue.front());
        m_queue.pop();
        return job;
    }
    
    bool empty() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    
private:
    std::queue<Job> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

// =============================================================================
// SEDA Tracing Tests
// =============================================================================

class SEDATracingTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock = std::make_unique<ThreadSafeMockBackend>();
        mock_ptr = mock.get();
        obs::set_backend(std::move(mock));
    }
    
    void TearDown() override {
        obs::shutdown();
    }
    
    // Simple mock backend that counts operations
    class ThreadSafeMockBackend : public IBackend {
    public:
        std::atomic<int> span_count{0};
        std::atomic<int> log_count{0};
        std::vector<std::string> span_names;
        std::mutex names_mutex;
        
        void shutdown() override {}
        
        std::unique_ptr<Span> create_span(std::string_view name, const Context&) override {
            span_count++;
            std::lock_guard<std::mutex> lock(names_mutex);
            span_names.push_back(std::string(name));
            return nullptr;
        }
        
        std::unique_ptr<Span> create_root_span(std::string_view name) override {
            span_count++;
            std::lock_guard<std::mutex> lock(names_mutex);
            span_names.push_back(std::string(name));
            return nullptr;
        }
        
        void log(Level, std::string_view, const Context&) override {
            log_count++;
        }
        
        std::shared_ptr<Counter> get_counter(std::string_view, std::string_view) override {
            return nullptr;
        }
        
        std::shared_ptr<Histogram> get_histogram(std::string_view, std::string_view) override {
            return nullptr;
        }
        
        std::shared_ptr<Gauge> get_gauge(std::string_view, std::string_view) override {
            return nullptr;
        }
    };
    
    std::unique_ptr<ThreadSafeMockBackend> mock;
    ThreadSafeMockBackend* mock_ptr;
};

TEST_F(SEDATracingTest, ContextFlowsThroughStages) {
    MockStage worker_stage;
    MockStage io_stage;
    
    // Network thread receives request
    obs::Context incoming = obs::Context::create();
    
    Job network_job = make_job(JobType::TASK, 1, std::string("GET /api/data"), incoming);
    
    // Submit to worker
    worker_stage.submit(network_job);
    
    // Worker takes job
    Job worker_job = worker_stage.take();
    
    // Same trace ID preserved
    EXPECT_EQ(worker_job.trace_ctx.trace_id.high, incoming.trace_id.high);
    EXPECT_EQ(worker_job.trace_ctx.trace_id.low, incoming.trace_id.low);
    
    // Worker creates child job for IO
    obs::SpanId worker_span{99999};
    Job io_job = make_job(JobType::TASK, worker_job.session_id, 
                          std::string("SELECT * FROM users"), 
                          worker_job.trace_ctx.child(worker_span));
    
    io_stage.submit(io_job);
    Job io_result = io_stage.take();
    
    // Still same trace ID
    EXPECT_EQ(io_result.trace_ctx.trace_id.high, incoming.trace_id.high);
    EXPECT_EQ(io_result.trace_ctx.trace_id.low, incoming.trace_id.low);
    
    // But parent span updated
    EXPECT_EQ(io_result.trace_ctx.span_id.value, worker_span.value);
}

TEST_F(SEDATracingTest, SpanPerStage) {
    obs::Context ctx = obs::Context::create();
    
    Job job = make_job(JobType::TASK, 42, std::any{}, ctx);
    
    // Network stage span
    obs::span("network_receive", job.trace_ctx);
    
    // Worker stage span
    obs::span("worker_process", job.trace_ctx);
    
    // IO stage span
    obs::span("io_query", job.trace_ctx);
    
    EXPECT_EQ(mock_ptr->span_count.load(), 3);
    
    std::lock_guard<std::mutex> lock(mock_ptr->names_mutex);
    EXPECT_EQ(mock_ptr->span_names[0], "network_receive");
    EXPECT_EQ(mock_ptr->span_names[1], "worker_process");
    EXPECT_EQ(mock_ptr->span_names[2], "io_query");
}

TEST_F(SEDATracingTest, LogsCorrelatedWithTrace) {
    obs::Context ctx = obs::Context::create();
    
    Job job = make_job(JobType::TASK, 100, std::any{}, ctx);
    
    // All logs within this request use same context
    obs::info("Request received", job.trace_ctx);
    obs::info("Processing started", job.trace_ctx);
    obs::info("Query executed", job.trace_ctx);
    obs::info("Response sent", job.trace_ctx);
    
    EXPECT_EQ(mock_ptr->log_count.load(), 4);
}

TEST_F(SEDATracingTest, ConcurrentRequestsIsolated) {
    std::vector<std::thread> threads;
    std::atomic<int> completed{0};
    
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&completed, i]() {
            // Each request gets its own context
            obs::Context ctx = obs::Context::create();
            
            Job job = make_job(JobType::TASK, static_cast<uint64_t>(i), i, ctx);
            
            obs::span("handle_request", job.trace_ctx);
            obs::info("Processing", job.trace_ctx);
            
            completed++;
        });
    }
    
    for (auto& t : threads) t.join();
    
    EXPECT_EQ(completed.load(), 10);
    EXPECT_EQ(mock_ptr->span_count.load(), 10);
    EXPECT_EQ(mock_ptr->log_count.load(), 10);
}

TEST_F(SEDATracingTest, BaggagePropagatesToAllStages) {
    obs::Context ctx = obs::Context::create();
    ctx.baggage["user_id"] = "user_42";
    ctx.baggage["request_id"] = "req_abc123";
    
    Job job1 = make_job(JobType::TASK, 1, std::any{}, ctx);
    
    // Simulate passing through stages
    Job job2 = job1;
    Job job3 = job2;
    
    // Baggage preserved through all stages
    EXPECT_EQ(job3.trace_ctx.baggage["user_id"], "user_42");
    EXPECT_EQ(job3.trace_ctx.baggage["request_id"], "req_abc123");
}

TEST_F(SEDATracingTest, W3CTraceparentEndToEnd) {
    // Incoming request with W3C traceparent header
    std::string incoming_header = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    obs::Context ctx = obs::Context::from_traceparent(incoming_header);
    
    Job job = make_job(JobType::TASK, 1, std::any{}, ctx);
    
    // Process through stages...
    obs::SpanId new_span{0xdeadbeef};
    obs::Context child = job.trace_ctx.child(new_span);
    
    // Generate outgoing header for downstream service
    std::string outgoing = child.to_traceparent();
    
    // Same trace ID in outgoing header
    EXPECT_NE(outgoing.find("4bf92f3577b34da6a3ce929d0e0e4736"), std::string::npos);
}

} // namespace obs::test
