/// @file ShortenLink.h
/// @brief ShortenLink use case - creates a new shortened URL

#pragma once

#include "Result.h"
#include "ILinkRepository.h"
#include "ICodeGenerator.h"
#include "DomainErrors.h"
#include <memory>
#include <string>
#include <optional>
#include <chrono>

namespace uri_shortener::application {

/**
 * @brief ShortenLink use case
 * 
 * Creates a new shortened URL from an original URL.
 * Generates a unique code, creates the link, and persists it.
 */
class ShortenLink {
public:
    /// Input for the use case
    struct Input {
        std::string original_url;
        std::optional<std::chrono::system_clock::duration> expires_after;
    };

    /// Output from the use case
    struct Output {
        std::string short_code;
        std::string original_url;
    };

    using Result = astra::outcome::Result<Output, domain::DomainError>;

    /**
     * @brief Constructor
     * @param repository Where to persist links
     * @param generator How to generate codes
     */
    ShortenLink(
        std::shared_ptr<domain::ILinkRepository> repository,
        std::shared_ptr<domain::ICodeGenerator> generator
    );

    /**
     * @brief Execute the use case
     * @param input The original URL and optional expiration
     * @return Ok(Output) with short code, or Err(DomainError)
     */
    Result execute(const Input& input);

private:
    std::shared_ptr<domain::ILinkRepository> m_repository;
    std::shared_ptr<domain::ICodeGenerator> m_generator;
};

} // namespace uri_shortener::application
