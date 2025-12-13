/// @file ResolveLink.h
/// @brief ResolveLink use case - retrieves original URL from short code

#pragma once

#include "Result.h"
#include "ILinkRepository.h"
#include "DomainErrors.h"
#include <memory>
#include <string>

namespace url_shortener::application {

/**
 * @brief ResolveLink use case
 * 
 * Resolves a short code to its original URL.
 * Returns error if link not found or expired.
 */
class ResolveLink {
public:
    struct Input {
        std::string short_code;
    };

    struct Output {
        std::string original_url;
    };

    using Result = astra::Result<Output, domain::DomainError>;

    explicit ResolveLink(std::shared_ptr<domain::ILinkRepository> repository);

    Result execute(const Input& input);

private:
    std::shared_ptr<domain::ILinkRepository> m_repository;
};

} // namespace url_shortener::application
