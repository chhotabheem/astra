#pragma once

#include "IResponse.h"

#include <IScopedResource.h>
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace astra::http2 {

class Http2ResponseWriter;

class Http2Response final : public astra::router::IResponse {
public:
  Http2Response() = default;
  explicit Http2Response(std::weak_ptr<Http2ResponseWriter> writer);

  Http2Response(const Http2Response &) = default;
  Http2Response &operator=(const Http2Response &) = default;
  Http2Response(Http2Response &&) noexcept = default;
  Http2Response &operator=(Http2Response &&) noexcept = default;

  ~Http2Response() override = default;

  void set_status(int code) noexcept override;
  void set_header(const std::string &key, const std::string &value) override;
  void write(const std::string &data) override;
  void close() override;
  [[nodiscard]] bool is_alive() const noexcept override;

  void add_scoped_resource(
      std::unique_ptr<astra::execution::IScopedResource> resource);

private:
  std::optional<int> m_status;
  std::map<std::string, std::string> m_headers;
  std::string m_body;
  std::weak_ptr<Http2ResponseWriter> m_writer;
  bool m_closed = false;
};

} // namespace astra::http2
