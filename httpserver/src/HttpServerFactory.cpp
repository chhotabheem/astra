#include "HttpServerFactory.h"
#include "mock/MockHttpServer.h"
#ifdef HAS_PROXYGEN
#include "proxygen/ProxygenHttpServer.h"
#endif
#include <stdexcept>
#include <iostream>

namespace httpserver {

std::unique_ptr<IHttpServer> HttpServerFactory::create(Type type) {
    switch (type) {
        case Type::MOCK:
            return std::make_unique<MockHttpServer>();
        case Type::PROXYGEN:
#ifdef HAS_PROXYGEN
            return std::make_unique<ProxygenHttpServer>();
#else
            // TODO: Return Proxygen implementation when available
            // For now, fallback to Mock with a warning
            std::cerr << "WARNING: Proxygen backend not available, using Mock backend" << std::endl;
            return std::make_unique<MockHttpServer>();
#endif
        default:
            throw std::runtime_error("Unknown server type");
    }
}

} // namespace httpserver
