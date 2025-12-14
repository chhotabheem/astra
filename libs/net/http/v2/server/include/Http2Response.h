#pragma once

#include "IResponse.h"
#include <memory>
#include <string>
#include <string_view>
#include <map>
#include <optional>
#include <IScopedResource.h>

namespace http2server {

class ResponseHandle;

class Response final : public router::IResponse {
public:
    Response() = default;
    explicit Response(std::weak_ptr<ResponseHandle> handle);
    
    Response(const Response&) = default;
    Response& operator=(const Response&) = default;
    Response(Response&&) noexcept = default;
    Response& operator=(Response&&) noexcept = default;
    
    ~Response() override = default;

    void set_status(int code) noexcept override;
    void set_header(std::string_view key, std::string_view value) override;
    void write(std::string_view data) override;
    void close() override;
    [[nodiscard]] bool is_alive() const noexcept override;
    
    void add_scoped_resource(std::unique_ptr<astra::execution::IScopedResource> resource);

private:
    std::optional<int> m_status;
    std::map<std::string, std::string> m_headers;
    std::string m_body;
    std::weak_ptr<ResponseHandle> m_handle;
    bool m_closed = false;
};

} // namespace http2server
