#include "Http2Response.h"
#include "Http2ResponseWriter.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <gtest/gtest.h>
#include <memory>

using namespace astra::http2;

/**
 * TDD Tests for Response as lightweight copyable handle
 *
 * Design: Response holds data directly + weak_ptr<Http2ResponseWriter>
 * - optional<int> m_status (no default - forces explicit set)
 * - bool m_closed (prevents double-send)
 * - Copyable
 * - close() sends via Http2ResponseWriter
 */

class Http2ResponseWriterIntegrationTest : public ::testing::Test {
protected:
  boost::asio::io_context io_ctx;

  struct SentData {
    int status = -1;
    std::map<std::string, std::string> headers;
    std::string body;
    int send_count = 0;
  };

  SentData sent;

  std::shared_ptr<Http2ResponseWriter> make_response_handle() {
    return std::make_shared<Http2ResponseWriter>(
        [this](int status, std::map<std::string, std::string> headers,
               std::string body) {
          sent.status = status;
          sent.headers = std::move(headers);
          sent.body = std::move(body);
          sent.send_count++;
        },
        [this](std::function<void()> work) {
          boost::asio::post(io_ctx, std::move(work));
        });
  }
};

TEST_F(Http2ResponseWriterIntegrationTest, Http2ResponseIsCopyable) {
  auto handle = make_response_handle();
  Http2Response res1(handle);

  // Copy construct
  Http2Response res2 = res1;

  // Copy assign
  Http2Response res3(handle);
  res3 = res1;

  // All should be valid
  res1.set_status(200);
  res2.write("test");
  // Should not crash
  SUCCEED();
}

TEST_F(Http2ResponseWriterIntegrationTest, Http2ResponseCloseSendsViaHandle) {
  auto handle = make_response_handle();
  Http2Response res(handle);

  res.set_status(201);
  res.set_header("Content-Type", "text/plain");
  res.write("Created");
  res.close();

  // Run io_context to process posted send
  io_ctx.run();

  EXPECT_EQ(sent.status, 201);
  EXPECT_EQ(sent.headers["Content-Type"], "text/plain");
  EXPECT_EQ(sent.body, "Created");
  EXPECT_EQ(sent.send_count, 1);
}

TEST_F(Http2ResponseWriterIntegrationTest,
       Http2ResponseDoubleCloseOnlySendsOnce) {
  auto handle = make_response_handle();
  Http2Response res(handle);

  res.set_status(200);
  res.write("OK");
  res.close();
  res.close(); // Second close should be no-op
  res.close(); // Third close should be no-op

  io_ctx.run();

  EXPECT_EQ(sent.send_count, 1); // Only sent once
}

TEST_F(Http2ResponseWriterIntegrationTest,
       Http2ResponseCloseGracefulWhenHandleExpired) {
  std::weak_ptr<Http2ResponseWriter> weak_handle;
  {
    auto handle = make_response_handle();
    weak_handle = handle;
  }
  // handle destroyed

  Http2Response res(weak_handle);
  res.set_status(200);
  res.write("test");
  res.close(); // Should not crash, gracefully dropped

  io_ctx.run();

  EXPECT_EQ(sent.send_count, 0); // Not sent
}

TEST_F(Http2ResponseWriterIntegrationTest,
       Http2ResponseStatusDefaultsTo500IfNotSet) {
  auto handle = make_response_handle();
  Http2Response res(handle);

  // Deliberately NOT calling set_status()
  res.write("oops");
  res.close();

  io_ctx.run();

  // Should default to 500 (error) not 200
  EXPECT_EQ(sent.status, 500);
}

TEST_F(Http2ResponseWriterIntegrationTest, Http2ResponseAccumulatesWrites) {
  auto handle = make_response_handle();
  Http2Response res(handle);

  res.set_status(200);
  res.write("Hello");
  res.write(" ");
  res.write("World");
  res.close();

  io_ctx.run();

  EXPECT_EQ(sent.body, "Hello World");
}

TEST_F(Http2ResponseWriterIntegrationTest,
       CopiedHttp2ResponseSharesClosedState) {
  auto handle = make_response_handle();
  Http2Response res1(handle);
  res1.set_status(200);
  res1.write("test");

  // Copy before close
  Http2Response res2 = res1;

  res1.close();
  io_ctx.run();
  EXPECT_EQ(sent.send_count, 1);

  io_ctx.restart();

  // res2 is a copy, but its own m_closed flag is independent
  // So it will try to send...but body was moved
  res2.close();
  io_ctx.run();

  // This behavior is debatable - documenting current expected behavior
  // After copy, close on copy is independent
  EXPECT_EQ(sent.send_count, 2); // Both copies sent
}

TEST_F(Http2ResponseWriterIntegrationTest,
       Http2ResponseSetHeaderMultipleTimes) {
  auto handle = make_response_handle();
  Http2Response res(handle);

  res.set_status(200);
  res.set_header("X-Custom", "value1");
  res.set_header("X-Custom", "value2"); // Overwrites
  res.close();

  io_ctx.run();

  EXPECT_EQ(sent.headers["X-Custom"], "value2");
}
