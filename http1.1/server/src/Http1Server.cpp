#include "Http1Server.hpp"
#include "Http1Request.hpp"
#include "Http1Response.hpp"
#include <iostream>

namespace http1 {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

Server::Server(const std::string& address, unsigned short port, int threads)
    : address_(address), port_(port), threads_(threads), ioc_(threads), acceptor_(ioc_) {
    
    auto const addr = net::ip::make_address(address);
    auto const endpoint = tcp::endpoint{addr, port};

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(net::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen(net::socket_base::max_listen_connections);
}

Server::~Server() {
    stop();
}

void Server::handle(Handler handler) {
    handler_ = std::move(handler);
}

void Server::run() {
    do_accept();

    for (int i = 0; i < threads_ - 1; ++i) {
        thread_pool_.emplace_back([this] {
            ioc_.run();
        });
    }
    ioc_.run();
}

void Server::stop() {
    ioc_.stop();
    for (auto& t : thread_pool_) {
        if (t.joinable()) t.join();
    }
    thread_pool_.clear();
}


class Session : public std::enable_shared_from_this<Session> {
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    Server::Handler handler_; // Copy of handler (std::function is cheap to copy usually, or use reference if lifetime guaranteed)
    // Actually Server lifetime outlives Session? Not necessarily if Server is destroyed.
    // But Handler is a function object.
    
public:
    Session(tcp::socket socket, Server::Handler handler)
        : socket_(std::move(socket)), handler_(std::move(handler)) {}

    void run() {
        do_read();
    }

private:
    void do_read() {
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, req_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);
                if(ec == http::error::end_of_stream)
                    return self->do_close();
                if(ec)
                    return; // Log error?

                self->process_request();
            });
    }

    void process_request() {
        auto self = shared_from_this();
        
        // Create abstractions
        // We need to keep request alive until response is sent?
        // The Request object copies data or views?
        // Http1Request uses boost::beast::http::request which owns data.
        // We move req_ into Request?
        // If we move req_, we can't use it for next loop if we wanted to reuse.
        // But for now let's assume one request per read or reset.
        
        // Construct Request wrapper
        // We need to be careful about moving req_ if we want to support pipelining/keep-alive later.
        // But Http1Request takes by value/move usually.
        
        // Let's look at Http1Request constructor: explicit Request(boost::beast::http::request<...> req);
        // It takes by value (copy or move).
        
        Request request(std::move(req_));
        
        auto send_lambda = [self](http::response<http::string_body> msg) {
            // The response object 'msg' needs to be kept alive during async_write
            // We need a shared_ptr to the message or capture it by value in the lambda.
            // http::async_write takes the message by const reference.
            // So the message must exist until completion.
            
            auto sp = std::make_shared<http::response<http::string_body>>(std::move(msg));
            
            http::async_write(self->socket_, *sp,
                [self, sp](beast::error_code ec, std::size_t bytes) {
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                    // In a real server we would check keep-alive and loop back to do_read()
                });
        };

        Response response(send_lambda);

        if (handler_) {
            handler_(request, response);
        } else {
            response.set_status(404);
            response.write("No handler configured");
            response.close();
        }
    }

    void do_close() {
        beast::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_send, ec);
    }
};

void Server::do_accept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        [this](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), handler_)->run();
            }
            do_accept();
        });
}

void Server::do_session(tcp::socket socket) {
    // Deprecated/Unused
}

} // namespace http1
