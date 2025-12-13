/// @file DomainErrors.h
/// @brief Domain-specific error types for URL Shortener

#pragma once

#include <string_view>

namespace url_shortener::domain {

/// Domain errors - pure domain terminology, no technology leakage
enum class DomainError {
    // Value Object Errors
    InvalidShortCode,      // Code doesn't meet format requirements
    InvalidUrl,            // URL is malformed or unsupported scheme
    
    // Entity Errors
    LinkNotFound,          // No link exists for given code
    LinkExpired,           // Link exists but has expired
    LinkAlreadyExists,     // Code collision
    
    // Service Errors
    CodeGenerationFailed   // Unable to generate unique code
};

/// Convert error to human-readable string (for logging/debugging)
constexpr std::string_view to_string(DomainError error) noexcept {
    switch (error) {
        case DomainError::InvalidShortCode:     return "Invalid short code format";
        case DomainError::InvalidUrl:           return "Invalid URL format";
        case DomainError::LinkNotFound:         return "Link not found";
        case DomainError::LinkExpired:          return "Link has expired";
        case DomainError::LinkAlreadyExists:    return "Link already exists";
        case DomainError::CodeGenerationFailed: return "Code generation failed";
        default:                                return "Unknown error";
    }
}

} // namespace url_shortener::domain
