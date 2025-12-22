#pragma once

#include "Result.h"
#include "ILinkRepository.h"
#include "DomainErrors.h"
#include <memory>
#include <string>

namespace uri_shortener::application {

class ResolveLink {
public:
    struct Input {
        std::string short_code;
    };

    struct Output {
        std::string original_url;
    };

    using Result = astra::outcome::Result<Output, domain::DomainError>;

    explicit ResolveLink(std::shared_ptr<domain::ILinkRepository> repository);

    Result execute(const Input& input);

private:
    std::shared_ptr<domain::ILinkRepository> m_repository;
};

}
