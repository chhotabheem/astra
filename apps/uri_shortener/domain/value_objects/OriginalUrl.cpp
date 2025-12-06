/// @file OriginalUrl.cpp
/// @brief OriginalUrl value object implementation

#include "domain/value_objects/OriginalUrl.h"
#include <algorithm>

namespace url_shortener::domain {

namespace {

/// Check if URL starts with valid scheme
bool has_valid_scheme(std::string_view url) {
    return url.starts_with("http://") || url.starts_with("https://");
}

/// Get the part after the scheme
std::string_view get_authority_and_path(std::string_view url) {
    if (url.starts_with("https://")) {
        return url.substr(8);
    }
    if (url.starts_with("http://")) {
        return url.substr(7);
    }
    return {};
}

/// Check for invalid URL characters
bool has_invalid_characters(std::string_view url) {
    // Basic check for obviously invalid characters
    constexpr std::string_view invalid_chars = " <>\"{}|\\^`";
    return std::any_of(url.begin(), url.end(), [&](char c) {
        return invalid_chars.find(c) != std::string_view::npos;
    });
}

/// Check if authority (host) portion is valid
bool has_valid_authority(std::string_view authority_and_path) {
    if (authority_and_path.empty()) {
        return false;
    }
    
    // Extract authority (before first /, ?, or #)
    auto end_of_authority = authority_and_path.find_first_of("/?#");
    std::string_view authority = (end_of_authority == std::string_view::npos) 
        ? authority_and_path 
        : authority_and_path.substr(0, end_of_authority);
    
    // Authority must have at least one character (the host)
    if (authority.empty()) {
        return false;
    }
    
    // Strip port if present
    auto port_pos = authority.rfind(':');
    if (port_pos != std::string_view::npos) {
        authority = authority.substr(0, port_pos);
    }
    
    // Host must have at least one character
    return !authority.empty();
}

} // namespace

OriginalUrl::CreateResult OriginalUrl::create(std::string_view raw) {
    // Check not empty
    if (raw.empty()) {
        return CreateResult::Err(DomainError::InvalidUrl);
    }

    // Check valid scheme
    if (!has_valid_scheme(raw)) {
        return CreateResult::Err(DomainError::InvalidUrl);
    }

    // Check for invalid characters
    if (has_invalid_characters(raw)) {
        return CreateResult::Err(DomainError::InvalidUrl);
    }

    // Check valid authority
    auto authority_and_path = get_authority_and_path(raw);
    if (!has_valid_authority(authority_and_path)) {
        return CreateResult::Err(DomainError::InvalidUrl);
    }

    return CreateResult::Ok(OriginalUrl(std::string(raw)));
}

OriginalUrl OriginalUrl::from_trusted(std::string raw) {
    return OriginalUrl(std::move(raw));
}

} // namespace url_shortener::domain
