#pragma once

#include <string>
#include <functional>
#include <memory>

namespace zookeeperclient {

class ZookeeperClient {
public:
    explicit ZookeeperClient(const std::string& connection_string);
    ~ZookeeperClient();

    // Delete copy constructor and assignment operator
    ZookeeperClient(const ZookeeperClient&) = delete;
    ZookeeperClient& operator=(const ZookeeperClient&) = delete;

    [[nodiscard]] bool create(const std::string& path, const std::string& value, bool ephemeral = false);
    [[nodiscard]] std::string get(const std::string& path);
    [[nodiscard]] bool exists(const std::string& path);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace zookeeperclient
