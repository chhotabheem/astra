/// @file DeleteLink.cpp
/// @brief DeleteLink use case implementation

#include "application/use_cases/DeleteLink.h"
#include "domain/value_objects/ShortCode.h"

namespace url_shortener::application {

DeleteLink::DeleteLink(std::shared_ptr<domain::ILinkRepository> repository)
    : m_repository(std::move(repository)) {}

DeleteLink::Result DeleteLink::execute(const Input& input) {
    // 1. Validate the short code
    auto code_result = domain::ShortCode::create(input.short_code);
    if (code_result.is_err()) {
        return Result::Err(code_result.error());
    }
    auto code = code_result.value();

    // 2. Remove from repository
    return m_repository->remove(code);
}

} // namespace url_shortener::application
