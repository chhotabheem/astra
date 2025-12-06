/// @file ILinkRepository.h
/// @brief Repository interface for ShortLink persistence

#pragma once

#include "Result.h"
#include "domain/entities/ShortLink.h"
#include "domain/value_objects/ShortCode.h"
#include "domain/errors/DomainErrors.h"
#include <memory>

namespace url_shortener::domain {

/**
 * @brief ILinkRepository - Port for link persistence
 * 
 * Abstraction for persisting and retrieving ShortLinks.
 * Speaks purely in domain terms - no technology leakage.
 * 
 * Implementations might use:
 * - In-memory storage (for testing)
 * - Cache + Database (production)
 * - Any other persistence mechanism
 */
class ILinkRepository {
public:
    virtual ~ILinkRepository() = default;

    // =========================================================================
    // Commands (write operations)
    // =========================================================================

    /**
     * @brief Save a new link
     * @param link The link to save
     * @return Ok on success, Err(LinkAlreadyExists) if code exists
     */
    virtual astra::Result<void, DomainError> save(const ShortLink& link) = 0;

    /**
     * @brief Remove a link by its code
     * @param code The code to remove
     * @return Ok on success, Err(LinkNotFound) if not found
     */
    virtual astra::Result<void, DomainError> remove(const ShortCode& code) = 0;

    // =========================================================================
    // Queries (read operations)
    // =========================================================================

    /**
     * @brief Find a link by its code
     * @param code The code to search for
     * @return Ok(ShortLink) if found, Err(LinkNotFound) if not
     */
    virtual astra::Result<ShortLink, DomainError> find_by_code(const ShortCode& code) = 0;

    /**
     * @brief Check if a code exists
     * @param code The code to check
     * @return true if exists, false otherwise
     */
    virtual bool exists(const ShortCode& code) = 0;
};

} // namespace url_shortener::domain
