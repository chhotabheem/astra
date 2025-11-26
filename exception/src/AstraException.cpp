#include "exception/AstraException.hpp"

namespace exception {

AstraException::AstraException(const std::string& message)
    : std::runtime_error(message) {}

} // namespace exception
