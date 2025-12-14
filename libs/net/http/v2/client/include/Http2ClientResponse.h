#pragma once

#include <string>
#include <map>
#include <memory>

namespace astra::http2 {

class ClientResponse {
public:
    ClientResponse();
    ~ClientResponse();

    // Copyable (cheap, shares impl)
    ClientResponse(const ClientResponse& other) = default;
    ClientResponse& operator=(const ClientResponse& other) = default;

    [[nodiscard]] int status_code() const;
    [[nodiscard]] const std::string& body() const;
    [[nodiscard]] std::string header(const std::string& name) const;
    [[nodiscard]] const std::map<std::string, std::string>& headers() const;

    // Internal implementation details
    class Impl;
    // Helper to create a response with an implementation
    explicit ClientResponse(std::shared_ptr<Impl> impl);

private:
    std::shared_ptr<Impl> m_impl;
};

} // namespace astra::http2
