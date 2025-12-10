#include <gtest/gtest.h>
#include <Job.h>
#include <Context.h>

namespace astra::execution::test {

inline Job make_job(JobType type, uint64_t session_id, std::any payload, obs::Context ctx) {
    Job job;
    job.type = type;
    job.session_id = session_id;
    job.payload = std::move(payload);
    job.trace_ctx = ctx;
    return job;
}

TEST(JobContextTest, JobHasTraceContext) {
    Job job{};
    EXPECT_FALSE(job.trace_ctx.is_valid());
}

TEST(JobContextTest, JobCarriesValidContext) {
    obs::Context ctx = obs::Context::create();
    
    Job job = make_job(JobType::TASK, 123, std::any{}, ctx);
    
    EXPECT_TRUE(job.trace_ctx.is_valid());
    EXPECT_EQ(job.trace_ctx.trace_id.high, ctx.trace_id.high);
    EXPECT_EQ(job.trace_ctx.trace_id.low, ctx.trace_id.low);
}

TEST(JobContextTest, ContextPreservedInPayload) {
    obs::Context original = obs::Context::create();
    
    Job job = make_job(JobType::TASK, 456, std::string("query result"), original);
    
    EXPECT_TRUE(job.trace_ctx.is_valid());
    EXPECT_EQ(std::any_cast<std::string>(job.payload), "query result");
}

TEST(JobContextTest, ChildContextPropagation) {
    obs::Context parent = obs::Context::create();
    obs::SpanId child_span{12345};
    obs::Context child = parent.child(child_span);
    
    Job parent_job = make_job(JobType::TASK, 100, std::any{}, parent);
    Job child_job = make_job(JobType::TASK, 100, std::any{}, child);
    
    EXPECT_EQ(parent_job.trace_ctx.trace_id.high, child_job.trace_ctx.trace_id.high);
    EXPECT_EQ(parent_job.trace_ctx.trace_id.low, child_job.trace_ctx.trace_id.low);
    EXPECT_NE(parent_job.trace_ctx.span_id.value, child_job.trace_ctx.span_id.value);
}

TEST(JobContextTest, BaggagePropagation) {
    obs::Context ctx = obs::Context::create();
    ctx.baggage["user_id"] = "user123";
    ctx.baggage["tenant"] = "acme";
    
    Job job = make_job(JobType::TASK, 789, std::any{}, ctx);
    
    EXPECT_EQ(job.trace_ctx.baggage["user_id"], "user123");
    EXPECT_EQ(job.trace_ctx.baggage["tenant"], "acme");
}

TEST(JobContextTest, TraceparentRoundTrip) {
    std::string traceparent = "00-0af7651916cd43dd8448eb211c80319c-b7ad6b7169203331-01";
    obs::Context incoming = obs::Context::from_traceparent(traceparent);
    
    Job job = make_job(JobType::TASK, 1000, std::any{}, incoming);
    
    std::string outgoing = job.trace_ctx.to_traceparent();
    EXPECT_NE(outgoing.find("0af7651916cd43dd8448eb211c80319c"), std::string::npos);
}

TEST(JobContextTest, JobCopyPreservesContext) {
    obs::Context ctx = obs::Context::create();
    
    Job original = make_job(JobType::TASK, 111, 42, ctx);
    Job copy = original;
    
    EXPECT_EQ(copy.trace_ctx.trace_id.high, original.trace_ctx.trace_id.high);
    EXPECT_EQ(copy.trace_ctx.trace_id.low, original.trace_ctx.trace_id.low);
}

TEST(JobContextTest, JobMovePreservesContext) {
    obs::Context ctx = obs::Context::create();
    auto original_trace_id = ctx.trace_id;
    
    Job original = make_job(JobType::TASK, 222, 99, ctx);
    Job moved = std::move(original);
    
    EXPECT_EQ(moved.trace_ctx.trace_id.high, original_trace_id.high);
    EXPECT_EQ(moved.trace_ctx.trace_id.low, original_trace_id.low);
}

} // namespace astra::execution::test

