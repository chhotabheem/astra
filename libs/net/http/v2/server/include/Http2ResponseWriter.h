#pragma once

#include <IScopedResource.h>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace astra::http2 {

class Http2ResponseWriter
    : public std::enable_shared_from_this<Http2ResponseWriter> {
public:
  using SendResponse =
      std::function<void(int status, std::map<std::string, std::string> headers,
                         std::string body)>;
  using PostWork = std::function<void(std::function<void()>)>;

  Http2ResponseWriter(SendResponse send_response, PostWork post_work);

  void send(int status, std::map<std::string, std::string> headers,
            std::string body);

  void mark_closed() noexcept;

  [[nodiscard]] bool is_alive() const noexcept;

  void add_scoped_resource(
      std::unique_ptr<astra::execution::IScopedResource> resource) {
    m_scoped_resources.push_back(std::move(resource));
  }

private:
  SendResponse m_send_response;
  PostWork m_post_work;
  std::atomic<bool> m_stream_alive{true};
  std::vector<std::unique_ptr<astra::execution::IScopedResource>>
      m_scoped_resources;
};

} // namespace astra::http2
