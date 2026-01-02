/// @file ResolveLink.cpp
/// @brief ResolveLink use case implementation

#include "ResolveLink.h"

#include "ShortCode.h"

namespace uri_shortener::application {

ResolveLink::ResolveLink(std::shared_ptr<domain::ILinkRepository> repository)
    : m_repository(std::move(repository)) {
}

ResolveLink::Result ResolveLink::execute(const Input &input) {
  // 1. Validate the short code
  auto code_result = domain::ShortCode::create(input.short_code);
  if (code_result.is_err()) {
    return Result::Err(code_result.error());
  }
  auto code = code_result.value();

  // 2. Find the link
  auto link_result = m_repository->find_by_code(code);
  if (link_result.is_err()) {
    return Result::Err(link_result.error());
  }
  auto link = link_result.value();

  // 3. Check if expired
  if (link.is_expired()) {
    return Result::Err(domain::DomainError::LinkExpired);
  }

  // 4. Return the original URL
  return Result::Ok(
      Output{.original_url = std::string(link.original().value())});
}

} // namespace uri_shortener::application
