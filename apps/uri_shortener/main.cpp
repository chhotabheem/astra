/// @file main.cpp
/// @brief URI Shortener entry point

#include "UriShortenerApp.h"
#include <iostream>

int main() {
    url_shortener::UriShortenerApp::Config config{
        .address = "0.0.0.0",
        .port = "8080"
    };

    auto result = url_shortener::UriShortenerApp::create(config);
    if (result.is_err()) {
        std::cerr << "Failed to start URI Shortener\n";
        return 1;
    }

    return result.value().run();
}
