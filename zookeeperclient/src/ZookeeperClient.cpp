#include "ZookeeperClient.hpp"
#include <iostream>
#include <vector>

#ifdef HAS_ZOOKEEPER
#include <zookeeper/zookeeper.h>
#endif

namespace zookeeperclient {

class ZookeeperClient::Impl {
public:
    std::string connection_string;
#ifdef HAS_ZOOKEEPER
    zhandle_t* zh = nullptr;
#endif

    Impl(const std::string& conn_str) : connection_string(conn_str) {
#ifdef HAS_ZOOKEEPER
        zh = zookeeper_init(conn_str.c_str(), nullptr, 10000, 0, nullptr, 0);
        if (!zh) {
            std::cerr << "Failed to connect to Zookeeper at " << conn_str << std::endl;
        }
#else
        std::cout << "[MOCK] Connecting to Zookeeper at " << conn_str << std::endl;
#endif
    }

    ~Impl() {
#ifdef HAS_ZOOKEEPER
        if (zh) {
            zookeeper_close(zh);
        }
#else
        std::cout << "[MOCK] Closing Zookeeper connection" << std::endl;
#endif
    }

    bool create(const std::string& path, const std::string& value, bool ephemeral) {
#ifdef HAS_ZOOKEEPER
        if (!zh) return false;
        int flags = ephemeral ? ZOO_EPHEMERAL : 0;
        int rc = zoo_create(zh, path.c_str(), value.c_str(), value.length(),
                          &ZOO_OPEN_ACL_UNSAFE, flags, nullptr, 0);
        return rc == ZOK;
#else
        std::cout << "[MOCK] Creating node " << path << " with value " << value << std::endl;
        return true;
#endif
    }

    std::string get(const std::string& path) {
#ifdef HAS_ZOOKEEPER
        if (!zh) return "";
        char buffer[1024];
        int buffer_len = sizeof(buffer);
        int rc = zoo_get(zh, path.c_str(), 0, buffer, &buffer_len, nullptr);
        if (rc == ZOK) {
            return std::string(buffer, buffer_len);
        }
        return "";
#else
        std::cout << "[MOCK] Getting value for node " << path << std::endl;
        return "mock_value";
#endif
    }

    bool exists(const std::string& path) {
#ifdef HAS_ZOOKEEPER
        if (!zh) return false;
        int rc = zoo_exists(zh, path.c_str(), 0, nullptr);
        return rc == ZOK;
#else
        std::cout << "[MOCK] Checking existence of node " << path << std::endl;
        return true;
#endif
    }
};

ZookeeperClient::ZookeeperClient(const std::string& connection_string)
    : impl_(std::make_unique<Impl>(connection_string)) {}

ZookeeperClient::~ZookeeperClient() = default;

bool ZookeeperClient::create(const std::string& path, const std::string& value, bool ephemeral) {
    return impl_->create(path, value, ephemeral);
}

std::string ZookeeperClient::get(const std::string& path) {
    return impl_->get(path);
}

bool ZookeeperClient::exists(const std::string& path) {
    return impl_->exists(path);
}

} // namespace zookeeperclient
