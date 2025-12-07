#pragma once

#include "Result.h"
#include "domain/entities/ShortLink.h"
#include "domain/value_objects/ShortCode.h"
#include "domain/errors/DomainErrors.h"
#include <memory>

namespace url_shortener::domain {

class ILinkRepository {
public:
    virtual ~ILinkRepository() = default;
    virtual astra::Result<void, DomainError> save(const ShortLink& link) = 0;
    virtual astra::Result<void, DomainError> remove(const ShortCode& code) = 0;
    virtual astra::Result<ShortLink, DomainError> find_by_code(const ShortCode& code) = 0;
    virtual bool exists(const ShortCode& code) = 0;
};

} // namespace url_shortener::domain
