/// @file DeleteLink.h
/// @brief DeleteLink use case - removes a shortened link

#pragma once

#include "DomainErrors.h"
#include "ILinkRepository.h"
#include "Result.h"

#include <memory>
#include <string>

namespace uri_shortener::application {

/**
 * @brief DeleteLink use case
 *
 * Removes a link by its short code.
 * Returns error if link not found.
 */
class DeleteLink {
public:
  struct Input {
    std::string short_code;
  };

  using Result = astra::outcome::Result<void, domain::DomainError>;

  explicit DeleteLink(std::shared_ptr<domain::ILinkRepository> repository);

  Result execute(const Input &input);

private:
  std::shared_ptr<domain::ILinkRepository> m_repository;
};

} // namespace uri_shortener::application
