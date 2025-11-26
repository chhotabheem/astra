#pragma once

#include <stdexcept>
#include <string>

namespace exception {

class AstraException : public std::runtime_error {
public:
    explicit AstraException(const std::string& message);
    ~AstraException() override = default;
};

} // namespace exception
