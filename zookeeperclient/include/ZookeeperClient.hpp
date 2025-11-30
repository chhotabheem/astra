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

    [[nodiscard]] bool create(std::string_view path, std::string_view value, bool ephemeral = false);
    [[nodiscard]] std::string get(std::string_view path);
    [[nodiscard]] bool exists(std::string_view path);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace zookeeperclient
