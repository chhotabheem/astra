#pragma once

#include "IHttpServer.h"
#include "IRouter.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

namespace httpserver {

class MockRouter : public IRouter {
public:
    void GET(const std::string& path, HttpHandler handler) override {
        std::cout << "MockRouter: Registered GET " << path << std::endl;
    }
    void POST(const std::string& path, HttpHandler handler) override {
        std::cout << "MockRouter: Registered POST " << path << std::endl;
    }
    void PUT(const std::string& path, HttpHandler handler) override {
        std::cout << "MockRouter: Registered PUT " << path << std::endl;
    }
    void DELETE(const std::string& path, HttpHandler handler) override {
        std::cout << "MockRouter: Registered DELETE " << path << std::endl;
    }
    void PATCH(const std::string& path, HttpHandler handler) override {
        std::cout << "MockRouter: Registered PATCH " << path << std::endl;
    }
    void use(MiddlewareHandler middleware) override {
        std::cout << "MockRouter: Registered middleware" << std::endl;
    }
};

class MockHttpServer : public IHttpServer {
public:
    void start(const std::string& host, int port) override {
        std::cout << "MockHttpServer: Starting on " << host << ":" << port << std::endl;
        
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            perror("socket failed");
            return;
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            perror("bind failed");
            return;
        }

        if (listen(server_fd, 3) < 0) {
            perror("listen failed");
            return;
        }

        running_ = true;
        std::cout << "MockHttpServer: Listening..." << std::endl;

        while (running_) {
            int new_socket = accept(server_fd, nullptr, nullptr);
            if (new_socket < 0) {
                if (running_) perror("accept failed");
                continue;
            }

            char buffer[1024] = {0};
            read(new_socket, buffer, 1024);
            // std::cout << "Received request:\n" << buffer << std::endl;

            const char* response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: 45\r\n"
                "\r\n"
                "{\"status\": \"ok\", \"message\": \"Hello from Mock\"}";

            write(new_socket, response, strlen(response));
            close(new_socket);
        }
        
        close(server_fd);
        std::cout << "MockHttpServer: Stopped" << std::endl;
    }

    void stop() override {
        running_ = false;
        // Note: This won't unblock accept() immediately in this simple implementation
    }

    IRouter& router() override {
        return router_;
    }

private:
    MockRouter router_;
    std::atomic<bool> running_{false};
};

} // namespace httpserver
