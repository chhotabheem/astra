/// @file ICodeGenerator.h
/// @brief Interface for generating unique short codes

#pragma once

#include "domain/value_objects/ShortCode.h"

namespace url_shortener::domain {

/**
 * @brief ICodeGenerator - Port for code generation
 * 
 * Abstraction for generating unique ShortCodes.
 * Speaks purely in domain terms - no technology leakage.
 * 
 * Implementations might use:
 * - Random generation
 * - Sequential/incremental IDs
 * - Hash-based generation
 * - External ID service
 */
class ICodeGenerator {
public:
    virtual ~ICodeGenerator() = default;

    /**
     * @brief Generate a new unique short code
     * @return A new ShortCode
     * 
     * @note Implementations must guarantee uniqueness within their scope.
     *       Collision handling is the caller's responsibility.
     */
    virtual ShortCode generate() = 0;
};

} // namespace url_shortener::domain
