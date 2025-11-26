#include "exception/AstraException.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

void test_exception_message() {
    std::string msg = "Test exception message";
    try {
        throw exception::AstraException(msg);
    } catch (const exception::AstraException& e) {
        assert(std::strcmp(e.what(), msg.c_str()) == 0);
        std::cout << "test_exception_message passed" << std::endl;
    } catch (...) {
        assert(false && "Caught wrong exception type");
    }
}

int main() {
    test_exception_message();
    std::cout << "All exception tests passed" << std::endl;
    return 0;
}
