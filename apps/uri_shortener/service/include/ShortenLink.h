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

class ShortenLink {
public:
    struct Input {
        std::string original_url;
        std::optional<std::chrono::system_clock::duration> expires_after;
    };

    struct Output {
        std::string short_code;
        std::string original_url;
    };

    using Result = astra::outcome::Result<Output, domain::DomainError>;

    ShortenLink(
        std::shared_ptr<domain::ILinkRepository> repository,
        std::shared_ptr<domain::ICodeGenerator> generator
    );

    Result execute(const Input& input);

private:
    std::shared_ptr<domain::ILinkRepository> m_repository;
    std::shared_ptr<domain::ICodeGenerator> m_generator;
};

}
