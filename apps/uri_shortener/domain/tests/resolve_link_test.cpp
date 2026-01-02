/// @file test_resolve_link.cpp
/// @brief TDD tests for ResolveLink use case

#include "ICodeGenerator.h"
#include "ILinkRepository.h"
#include "ResolveLink.h"
#include "ShortenLink.h"

#include <gtest/gtest.h>
#include <map>
#include <memory>

namespace uri_shortener::application::test {

using namespace uri_shortener::domain;

// Reuse mocks from test_shorten_link.cpp (would normally be in shared header)
class MockCodeGenerator : public ICodeGenerator {
public:
  ShortCode generate() override {
    return ShortCode::from_trusted("res" + std::to_string(m_counter++));
  }

private:
  int m_counter = 100;
};

class MockLinkRepository : public ILinkRepository {
public:
  astra::outcome::Result<void, DomainError>
  save(const ShortLink &link) override {
    auto code_str = std::string(link.code().value());
    m_links.insert_or_assign(code_str, link);
    return astra::outcome::Result<void, DomainError>::Ok();
  }

  astra::outcome::Result<void, DomainError>
  remove(const ShortCode &code) override {
    auto code_str = std::string(code.value());
    if (!m_links.count(code_str)) {
      return astra::outcome::Result<void, DomainError>::Err(
          DomainError::LinkNotFound);
    }
    m_links.erase(code_str);
    return astra::outcome::Result<void, DomainError>::Ok();
  }

  astra::outcome::Result<ShortLink, DomainError>
  find_by_code(const ShortCode &code) override {
    auto code_str = std::string(code.value());
    if (!m_links.count(code_str)) {
      return astra::outcome::Result<ShortLink, DomainError>::Err(
          DomainError::LinkNotFound);
    }
    return astra::outcome::Result<ShortLink, DomainError>::Ok(
        m_links.at(code_str));
  }

  bool exists(const ShortCode &code) override {
    return m_links.count(std::string(code.value())) > 0;
  }

private:
  std::map<std::string, ShortLink> m_links;
};

class ResolveLinkTest : public ::testing::Test {
protected:
  void SetUp() override {
    m_repository = std::make_shared<MockLinkRepository>();
    m_generator = std::make_shared<MockCodeGenerator>();
    m_shorten = std::make_unique<ShortenLink>(m_repository, m_generator);
    m_resolve = std::make_unique<ResolveLink>(m_repository);
  }

  std::shared_ptr<MockLinkRepository> m_repository;
  std::shared_ptr<MockCodeGenerator> m_generator;
  std::unique_ptr<ShortenLink> m_shorten;
  std::unique_ptr<ResolveLink> m_resolve;
};

TEST_F(ResolveLinkTest, Execute_WithExistingCode_ReturnsOriginalUrl) {
  // First create a link
  auto shorten_result =
      m_shorten->execute({.original_url = "https://example.com/long"});
  ASSERT_TRUE(shorten_result.is_ok());
  auto short_code = shorten_result.value().short_code;

  // Now resolve it
  auto resolve_result = m_resolve->execute({.short_code = short_code});

  ASSERT_TRUE(resolve_result.is_ok());
  EXPECT_EQ(resolve_result.value().original_url, "https://example.com/long");
}

TEST_F(ResolveLinkTest, Execute_WithNonExistentCode_ReturnsNotFound) {
  ResolveLink::Input input{.short_code = "abc123"};

  auto result = m_resolve->execute(input);

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.error(), DomainError::LinkNotFound);
}

TEST_F(ResolveLinkTest, Execute_WithInvalidCode_ReturnsError) {
  ResolveLink::Input input{.short_code = "ab"}; // Too short

  auto result = m_resolve->execute(input);

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.error(), DomainError::InvalidShortCode);
}

} // namespace uri_shortener::application::test
