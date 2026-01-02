#pragma once

#include "ILinkRepository.h"

#include <map>
#include <mutex>

namespace uri_shortener::infrastructure {

class InMemoryLinkRepository : public domain::ILinkRepository {
public:
  astra::outcome::Result<void, domain::DomainError>
  save(const domain::ShortLink &link) override {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto code_str = std::string(link.code().value());
    if (m_links.count(code_str)) {
      return astra::outcome::Result<void, domain::DomainError>::Err(
          domain::DomainError::LinkAlreadyExists);
    }
    m_links.insert_or_assign(code_str, link);
    return astra::outcome::Result<void, domain::DomainError>::Ok();
  }

  astra::outcome::Result<void, domain::DomainError>
  remove(const domain::ShortCode &code) override {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto code_str = std::string(code.value());
    if (!m_links.count(code_str)) {
      return astra::outcome::Result<void, domain::DomainError>::Err(
          domain::DomainError::LinkNotFound);
    }
    m_links.erase(code_str);
    return astra::outcome::Result<void, domain::DomainError>::Ok();
  }

  astra::outcome::Result<domain::ShortLink, domain::DomainError>
  find_by_code(const domain::ShortCode &code) override {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto code_str = std::string(code.value());
    auto it = m_links.find(code_str);
    if (it == m_links.end()) {
      return astra::outcome::Result<domain::ShortLink, domain::DomainError>::
          Err(domain::DomainError::LinkNotFound);
    }
    return astra::outcome::Result<domain::ShortLink, domain::DomainError>::Ok(
        it->second);
  }

  bool exists(const domain::ShortCode &code) override {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_links.count(std::string(code.value())) > 0;
  }

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

} // namespace uri_shortener::infrastructure
