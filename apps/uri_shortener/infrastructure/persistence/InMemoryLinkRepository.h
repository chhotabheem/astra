/// @file InMemoryLinkRepository.h
/// @brief In-memory implementation of ILinkRepository for testing

#pragma once

#include "domain/ports/ILinkRepository.h"
#include <map>
#include <mutex>

namespace url_shortener::infrastructure {

/**
 * @brief In-memory link repository for testing
 * Thread-safe implementation using mutex.
 */
class InMemoryLinkRepository : public domain::ILinkRepository {
public:
    astra::Result<void, domain::DomainError> save(const domain::ShortLink& link) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto code_str = std::string(link.code().value());
        if (m_links.count(code_str)) {
            return astra::Result<void, domain::DomainError>::Err(domain::DomainError::LinkAlreadyExists);
        }
        m_links.insert_or_assign(code_str, link);
        return astra::Result<void, domain::DomainError>::Ok();
    }

    astra::Result<void, domain::DomainError> remove(const domain::ShortCode& code) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto code_str = std::string(code.value());
        if (!m_links.count(code_str)) {
            return astra::Result<void, domain::DomainError>::Err(domain::DomainError::LinkNotFound);
        }
        m_links.erase(code_str);
        return astra::Result<void, domain::DomainError>::Ok();
    }

    astra::Result<domain::ShortLink, domain::DomainError> find_by_code(const domain::ShortCode& code) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto code_str = std::string(code.value());
        auto it = m_links.find(code_str);
        if (it == m_links.end()) {
            return astra::Result<domain::ShortLink, domain::DomainError>::Err(domain::DomainError::LinkNotFound);
        }
        return astra::Result<domain::ShortLink, domain::DomainError>::Ok(it->second);
    }

    bool exists(const domain::ShortCode& code) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_links.count(std::string(code.value())) > 0;
    }

    // Test helpers
    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_links.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_links.clear();
    }

private:
    mutable std::mutex m_mutex;
    std::map<std::string, domain::ShortLink> m_links;
};

} // namespace url_shortener::infrastructure
