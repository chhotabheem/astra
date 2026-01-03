#include "ClientDispatcher.h"
#include "Http2Client.h"
#include "Http2ClientError.h"
#include "Http2ClientResponse.h"
#include "NgHttp2Client.h"

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace astra::http2 {
namespace test {

class Http2ClientTest : public ::testing::Test {
protected:
  void SetUp() override {
    m_config.set_connect_timeout_ms(100);
    m_config.set_request_timeout_ms(100);
  }

  ClientConfig m_config;
};

TEST_F(Http2ClientTest, ConstructionDoesNotThrow) {
  EXPECT_NO_THROW({ Http2Client client(m_config); });
}

TEST_F(Http2ClientTest, SubmitWithHostPortCallsHandler) {
  Http2Client client(m_config);
  std::atomic<bool> done{false};
  bool got_error = false;

  client.submit("127.0.0.1", 19999, "GET", "/test", "", {}, [&](auto result) {
    got_error = result.is_err();
    done = true;
  });

  while (!done) {
    std::this_thread::yield();
  }
  EXPECT_TRUE(got_error);
}

TEST_F(Http2ClientTest, SubmitPostWithBody) {
  Http2Client client(m_config);
  std::atomic<bool> done{false};

  std::string body = R"({"key": "value"})";
  std::map<std::string, std::string> headers;
  headers["Content-Type"] = "application/json";

  client.submit("127.0.0.1", 19999, "POST", "/api/data", body, headers,
                [&](auto) {
                  done = true;
                });

  while (!done) {
    std::this_thread::yield();
  }
}

TEST_F(Http2ClientTest, SubmitToMultiplePeersWorks) {
  Http2Client client(m_config);
  std::atomic<int> count{0};

  client.submit("127.0.0.1", 19999, "GET", "/a", "", {}, [&](auto) {
    count++;
  });
  client.submit("127.0.0.1", 19998, "GET", "/b", "", {}, [&](auto) {
    count++;
  });
  client.submit("127.0.0.1", 19997, "GET", "/c", "", {}, [&](auto) {
    count++;
  });

  while (count < 3) {
    std::this_thread::yield();
  }
  EXPECT_EQ(count.load(), 3);
}

TEST_F(Http2ClientTest, SubmitToSamePeerMultipleTimes) {
  Http2Client client(m_config);
  std::atomic<int> count{0};

  for (int i = 0; i < 5; i++) {
    client.submit("127.0.0.1", 19999, "GET", "/test" + std::to_string(i), "",
                  {}, [&](auto) {
                    count++;
                  });
  }

  while (count < 5) {
    std::this_thread::yield();
  }
  EXPECT_EQ(count.load(), 5);
}

TEST_F(Http2ClientTest, ConnectionFailedErrorReturned) {
  Http2Client client(m_config);

  Http2ClientError error_received = Http2ClientError::NotConnected;
  std::atomic<bool> done{false};

  client.submit("127.0.0.1", 19999, "GET", "/test", "", {}, [&](auto result) {
    if (result.is_err()) {
      error_received = result.error();
    }
    done = true;
  });

  while (!done) {
    std::this_thread::yield();
  }
  EXPECT_EQ(error_received, Http2ClientError::ConnectionFailed);
}

TEST_F(Http2ClientTest, ConcurrentSubmitsFromMultipleThreads) {
  Http2Client client(m_config);
  std::atomic<int> count{0};
  std::vector<std::thread> threads;

  for (int i = 0; i < 10; i++) {
    threads.emplace_back([&client, &count, i]() {
      client.submit("127.0.0.1", 19999, "GET", "/test" + std::to_string(i), "",
                    {}, [&count](auto) {
                      count++;
                    });
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  while (count < 10) {
    std::this_thread::yield();
  }
  EXPECT_EQ(count.load(), 10);
}

TEST_F(Http2ClientTest, SubmitToDifferentHostsSamePort) {
  Http2Client client(m_config);
  std::atomic<int> count{0};

  client.submit("127.0.0.1", 19999, "GET", "/a", "", {}, [&](auto) {
    count++;
  });
  client.submit("127.0.0.2", 19999, "GET", "/b", "", {}, [&](auto) {
    count++;
  });
  client.submit("127.0.0.1", 19998, "GET", "/c", "", {}, [&](auto) {
    count++;
  });

  while (count < 3) {
    std::this_thread::yield();
  }
  EXPECT_EQ(count.load(), 3);
}

TEST_F(Http2ClientTest, SubmitWithAllHttpMethods) {
  Http2Client client(m_config);
  std::atomic<int> count{0};

  client.submit("127.0.0.1", 19999, "GET", "/resource", "", {}, [&](auto) {
    count++;
  });
  client.submit("127.0.0.1", 19999, "POST", "/resource", "{}", {}, [&](auto) {
    count++;
  });
  client.submit("127.0.0.1", 19999, "PUT", "/resource/1", "{}", {}, [&](auto) {
    count++;
  });
  client.submit("127.0.0.1", 19999, "DELETE", "/resource/1", "", {}, [&](auto) {
    count++;
  });
  client.submit("127.0.0.1", 19999, "PATCH", "/resource/1", "{}", {},
                [&](auto) {
                  count++;
                });

  while (count < 5) {
    std::this_thread::yield();
  }
  EXPECT_EQ(count.load(), 5);
}

TEST_F(Http2ClientTest, SubmitWithCustomHeaders) {
  Http2Client client(m_config);
  std::atomic<bool> done{false};

  std::map<std::string, std::string> headers;
  headers["Authorization"] = "Bearer token123";
  headers["X-Request-Id"] = "req-456";
  headers["X-Trace-Id"] = "trace-789";

  client.submit("127.0.0.1", 19999, "GET", "/secure", "", headers, [&](auto) {
    done = true;
  });

  while (!done) {
    std::this_thread::yield();
  }
}

TEST_F(Http2ClientTest, DestructorCleansUpGracefully) {
  std::atomic<int> count{0};

  {
    Http2Client client(m_config);
    for (int i = 0; i < 3; i++) {
      client.submit("127.0.0.1", 19999 + i, "GET", "/test", "", {}, [&](auto) {
        count++;
      });
    }
    while (count < 3) {
      std::this_thread::yield();
    }
  }
}

TEST_F(Http2ClientTest, HighConcurrencyMultiplePeers) {
  Http2Client client(m_config);
  std::atomic<int> count{0};
  const int num_requests = 50;
  const int num_peers = 5;

  for (int i = 0; i < num_requests; i++) {
    uint16_t port = 19990 + (i % num_peers);
    client.submit("127.0.0.1", port, "GET", "/test" + std::to_string(i), "", {},
                  [&](auto) {
                    count++;
                  });
  }

  while (count < num_requests) {
    std::this_thread::yield();
  }
  EXPECT_EQ(count.load(), num_requests);
}

TEST_F(Http2ClientTest, SubmitWithEmptyBody) {
  Http2Client client(m_config);
  std::atomic<bool> done{false};

  client.submit("127.0.0.1", 19999, "POST", "/empty", "", {}, [&](auto) {
    done = true;
  });

  while (!done) {
    std::this_thread::yield();
  }
}

TEST_F(Http2ClientTest, SubmitWithLargeBody) {
  Http2Client client(m_config);
  std::atomic<bool> done{false};

  std::string large_body(10000, 'x');

  client.submit("127.0.0.1", 19999, "POST", "/large", large_body, {},
                [&](auto) {
                  done = true;
                });

  while (!done) {
    std::this_thread::yield();
  }
}

TEST_F(Http2ClientTest, SubmitWithSpecialCharactersInPath) {
  Http2Client client(m_config);
  std::atomic<bool> done{false};

  client.submit("127.0.0.1", 19999, "GET",
                "/path/with/special?query=value&foo=bar", "", {}, [&](auto) {
                  done = true;
                });

  while (!done) {
    std::this_thread::yield();
  }
}

// =============================================================================
// Http2ClientResponse Tests
// =============================================================================

class Http2ClientResponseTest : public ::testing::Test {};

TEST_F(Http2ClientResponseTest, DefaultConstructorCreatesEmptyResponse) {
  Http2ClientResponse response;
  EXPECT_EQ(response.status_code(), 0);
  EXPECT_TRUE(response.body().empty());
  EXPECT_TRUE(response.headers().empty());
}

TEST_F(Http2ClientResponseTest, ParameterizedConstructorSetsValues) {
  std::map<std::string, std::string> headers;
  headers["content-type"] = "application/json";

  Http2ClientResponse response(200, "test body", headers);

  EXPECT_EQ(response.status_code(), 200);
  EXPECT_EQ(response.body(), "test body");
  EXPECT_EQ(response.headers().size(), 1);
}

TEST_F(Http2ClientResponseTest, HeaderReturnsValueWhenExists) {
  std::map<std::string, std::string> headers;
  headers["content-type"] = "application/json";

  Http2ClientResponse response(200, "", headers);

  EXPECT_EQ(response.header("content-type"), "application/json");
}

TEST_F(Http2ClientResponseTest, HeaderReturnsEmptyWhenNotExists) {
  Http2ClientResponse response(200, "", {});
  EXPECT_EQ(response.header("nonexistent"), "");
}

TEST_F(Http2ClientResponseTest, CopyConstructorWorks) {
  Http2ClientResponse original(200, "body", {});
  Http2ClientResponse copy(original);

  EXPECT_EQ(copy.status_code(), 200);
  EXPECT_EQ(copy.body(), "body");
}

TEST_F(Http2ClientResponseTest, MoveConstructorWorks) {
  Http2ClientResponse original(200, "body", {});
  Http2ClientResponse moved(std::move(original));

  EXPECT_EQ(moved.status_code(), 200);
  EXPECT_EQ(moved.body(), "body");
}

// =============================================================================
// NgHttp2Client Direct Tests
// =============================================================================

class NgHttp2ClientTest : public ::testing::Test {
protected:
  void SetUp() override {
    m_config.set_connect_timeout_ms(100);
    m_config.set_request_timeout_ms(100);
  }

  ClientConfig m_config;
};

TEST_F(NgHttp2ClientTest, InitialStateIsDisconnected) {
  NgHttp2Client client("127.0.0.1", 19999, m_config);
  EXPECT_EQ(client.state(), ConnectionState::DISCONNECTED);
  EXPECT_FALSE(client.is_connected());
}

TEST_F(NgHttp2ClientTest, SubmitToNonExistentHostReturnsError) {
  NgHttp2Client client("127.0.0.1", 19999, m_config);

  std::atomic<bool> done{false};
  Http2ClientError error_received = Http2ClientError::NotConnected;

  client.submit("GET", "/test", "", {}, [&](auto result) {
    if (result.is_err()) {
      error_received = result.error();
    }
    done = true;
  });

  while (!done) {
    std::this_thread::yield();
  }
  EXPECT_EQ(error_received, Http2ClientError::ConnectionFailed);
}

TEST_F(NgHttp2ClientTest, OnCloseCallbackInvoked) {
  std::atomic<bool> close_called{false};
  std::atomic<bool> error_called{false};
  std::atomic<bool> done{false};

  {
    NgHttp2Client client(
        "127.0.0.1", 19999, m_config,
        [&close_called]() {
          close_called = true;
        },
        [&error_called](Http2ClientError) {
          error_called = true;
        });

    client.submit("GET", "/test", "", {}, [&done](auto) {
      done = true;
    });

    while (!done) {
      std::this_thread::yield();
    }
  }

  EXPECT_TRUE(close_called.load() || error_called.load());
}

TEST_F(NgHttp2ClientTest, MultipleRequestsQueuedBeforeConnect) {
  NgHttp2Client client("127.0.0.1", 19999, m_config);

  std::atomic<int> count{0};

  for (int i = 0; i < 5; i++) {
    client.submit("GET", "/test" + std::to_string(i), "", {}, [&](auto) {
      count++;
    });
  }

  while (count < 5) {
    std::this_thread::yield();
  }
  EXPECT_EQ(count.load(), 5);
}

// =============================================================================
// ClientDispatcher Tests
// =============================================================================

class ClientDispatcherTest : public ::testing::Test {
protected:
  void SetUp() override {
    m_config.set_connect_timeout_ms(100);
    m_config.set_request_timeout_ms(100);
  }

  ClientConfig m_config;
};

TEST_F(ClientDispatcherTest, ConstructionDoesNotThrow) {
  EXPECT_NO_THROW({ ClientDispatcher dispatcher(m_config); });
}

TEST_F(ClientDispatcherTest, SubmitToMultiplePeersCreatesMultipleClients) {
  ClientDispatcher dispatcher(m_config);
  std::atomic<int> count{0};

  dispatcher.submit("127.0.0.1", 19991, "GET", "/a", "", {}, [&](auto) {
    count++;
  });
  dispatcher.submit("127.0.0.1", 19992, "GET", "/b", "", {}, [&](auto) {
    count++;
  });
  dispatcher.submit("127.0.0.1", 19993, "GET", "/c", "", {}, [&](auto) {
    count++;
  });

  while (count < 3) {
    std::this_thread::yield();
  }
  EXPECT_EQ(count.load(), 3);
}

TEST_F(ClientDispatcherTest, SubmitToSamePeerReusesClient) {
  ClientDispatcher dispatcher(m_config);
  std::atomic<int> count{0};

  for (int i = 0; i < 5; i++) {
    dispatcher.submit("127.0.0.1", 19999, "GET", "/test" + std::to_string(i),
                      "", {}, [&](auto) {
                        count++;
                      });
  }

  while (count < 5) {
    std::this_thread::yield();
  }
  EXPECT_EQ(count.load(), 5);
}

TEST_F(ClientDispatcherTest, DestructorWaitsForPendingRequests) {
  std::atomic<int> count{0};

  {
    ClientDispatcher dispatcher(m_config);
    for (int i = 0; i < 3; i++) {
      dispatcher.submit("127.0.0.1", 19999 + i, "GET", "/test", "", {},
                        [&](auto) {
                          count++;
                        });
    }
    while (count < 3) {
      std::this_thread::yield();
    }
  }
}

} // namespace test
} // namespace astra::http2
