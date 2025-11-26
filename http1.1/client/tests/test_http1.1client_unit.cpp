#include "Http1Client.hpp"
#include <iostream>
#include <cassert>

void test_connection_refused() {
    http1::Client client;
    // Connect to a port that is likely closed
    auto res = client.get("127.0.0.1", "1", "/"); 
    assert(res.status_code == 500);
    std::cout << "test_connection_refused passed" << std::endl;
}

void test_invalid_host() {
    http1::Client client;
    auto res = client.get("invalid.host.local", "80", "/");
    assert(res.status_code == 500);
    std::cout << "test_invalid_host passed" << std::endl;
}

int main() {
    test_connection_refused();
    test_invalid_host();
    std::cout << "All http1.1client tests passed" << std::endl;
    return 0;
}
