#include "AstraException.h"

namespace exception {

AstraException::AstraException(const std::string& message)
    : std::runtime_error(message) {}

} // namespace exception
