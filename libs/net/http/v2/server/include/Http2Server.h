#pragma once

#include <string>
#include <memory>
#include <functional>
#include "Router.h"
#include "http2server.pb.h"
#include "Http2ServerError.h"
#include <Result.h>

namespace astra::http2 {

class Http2Request;
class Http2Response;

class Http2Server {
public:
    explicit Http2Server(const ServerConfig& config);
    ~Http2Server();

    Http2Server(const Http2Server&) = delete;
    Http2Server& operator=(const Http2Server&) = delete;

    using Handler = astra::router::Handler;

    void handle(const std::string& method, const std::string& path, Handler handler);
    
    astra::outcome::Result<void, Http2ServerError> start();
    astra::outcome::Result<void, Http2ServerError> join();
    astra::outcome::Result<void, Http2ServerError> stop();

    [[nodiscard]] astra::router::Router& router() { return m_router; }

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    astra::router::Router m_router;
};

} // namespace astra::http2
