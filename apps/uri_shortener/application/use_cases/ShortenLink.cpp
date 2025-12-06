/// @file ShortenLink.cpp
/// @brief ShortenLink use case implementation

#include "application/use_cases/ShortenLink.h"
#include "domain/value_objects/OriginalUrl.h"
#include "domain/value_objects/ExpirationPolicy.h"
#include "domain/entities/ShortLink.h"

namespace url_shortener::application {

ShortenLink::ShortenLink(
    std::shared_ptr<domain::ILinkRepository> repository,
    std::shared_ptr<domain::ICodeGenerator> generator
)
    : m_repository(std::move(repository))
    , m_generator(std::move(generator)) {}

ShortenLink::Result ShortenLink::execute(const Input& input) {
    // 1. Validate the original URL
    auto url_result = domain::OriginalUrl::create(input.original_url);
    if (url_result.is_err()) {
        return Result::Err(url_result.error());
    }
    auto original_url = url_result.value();

    // 2. Generate a unique code
    auto code = m_generator->generate();

    // 3. Create expiration policy
    auto expiration = input.expires_after.has_value()
        ? domain::ExpirationPolicy::after(input.expires_after.value())
        : domain::ExpirationPolicy::never();

    // 4. Create the link
    auto link_result = domain::ShortLink::create(code, original_url, expiration);
    if (link_result.is_err()) {
        return Result::Err(link_result.error());
    }
    auto link = link_result.value();

    // 5. Persist
    auto save_result = m_repository->save(link);
    if (save_result.is_err()) {
        return Result::Err(save_result.error());
    }

    // 6. Return output
    return Result::Ok(Output{
        .short_code = std::string(link.code().value()),
        .original_url = std::string(link.original().value())
    });
}

} // namespace url_shortener::application
