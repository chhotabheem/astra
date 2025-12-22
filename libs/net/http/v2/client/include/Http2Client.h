#pragma once

#include <string>
#include <functional>
#include <memory>
#include <map>
#include <Result.h>
#include "Http2ClientResponse.h"
#include "Http2ClientError.h"
#include "http2client.pb.h"

namespace astra::http2 {

using ResponseHandler = std::function<void(astra::outcome::Result<Http2ClientResponse, Http2ClientError>)>;

class Http2Client {
public:
    explicit Http2Client(const ClientConfig& config);
    ~Http2Client();

    Http2Client(const Http2Client&) = delete;
    Http2Client& operator=(const Http2Client&) = delete;

    void submit(const std::string& host, uint16_t port,
                const std::string& method, const std::string& path,
                const std::string& body,
                const std::map<std::string, std::string>& headers,
                ResponseHandler handler);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}
