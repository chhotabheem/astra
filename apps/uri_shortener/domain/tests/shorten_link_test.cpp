/// @file test_shorten_link.cpp
/// @brief TDD tests for ShortenLink use case

#include "ICodeGenerator.h"
#include "ILinkRepository.h"
#include "ShortenLink.h"

#include <gtest/gtest.h>
#include <memory>

namespace uri_shortener::application::test {

using namespace uri_shortener::domain;

// =============================================================================
// Mock Implementations
// =============================================================================

class MockCodeGenerator : public ICodeGenerator {
public:
  ShortCode generate() override {
    return ShortCode::from_trusted("gen" + std::to_string(m_counter++));
  }

private:
  int m_counter = 100;
};

class MockLinkRepository : public ILinkRepository {
public:
  astra::outcome::Result<void, DomainError>
  save(const ShortLink &link) override {
    auto code_str = std::string(link.code().value());
    if (m_links.count(code_str)) {
      return astra::outcome::Result<void, DomainError>::Err(
          DomainError::LinkAlreadyExists);
    }
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

  size_t size() const {
    return m_links.size();
  }

private:
  std::map<std::string, ShortLink> m_links;
};

// =============================================================================
// Use Case Tests
// =============================================================================

class ShortenLinkTest : public ::testing::Test {
protected:
  void SetUp() override {
    m_repository = std::make_shared<MockLinkRepository>();
    m_generator = std::make_shared<MockCodeGenerator>();
    m_use_case = std::make_unique<ShortenLink>(m_repository, m_generator);
  }

  std::shared_ptr<MockLinkRepository> m_repository;
  std::shared_ptr<MockCodeGenerator> m_generator;
  std::unique_ptr<ShortenLink> m_use_case;
};

TEST_F(ShortenLinkTest, Execute_WithValidUrl_CreatesLink) {
  ShortenLink::Input input{.original_url = "https://example.com/long/path"};

  auto result = m_use_case->execute(input);

  ASSERT_TRUE(result.is_ok());
  auto output = result.value();
  EXPECT_FALSE(output.short_code.empty());
  EXPECT_EQ(output.original_url, "https://example.com/long/path");
}

TEST_F(ShortenLinkTest, Execute_SavesLinkToRepository) {
  ShortenLink::Input input{.original_url = "https://example.com"};

  auto result = m_use_case->execute(input);

  ASSERT_TRUE(result.is_ok());
  EXPECT_EQ(m_repository->size(), 1);
}

TEST_F(ShortenLinkTest, Execute_WithInvalidUrl_ReturnsError) {
  ShortenLink::Input input{.original_url = "not-a-valid-url"};

  auto result = m_use_case->execute(input);

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.error(), DomainError::InvalidUrl);
}

TEST_F(ShortenLinkTest, Execute_GeneratesUniqueCode) {
  ShortenLink::Input input1{.original_url = "https://example1.com"};
  ShortenLink::Input input2{.original_url = "https://example2.com"};

  auto result1 = m_use_case->execute(input1);
  auto result2 = m_use_case->execute(input2);

  ASSERT_TRUE(result1.is_ok());
  ASSERT_TRUE(result2.is_ok());
  EXPECT_NE(result1.value().short_code, result2.value().short_code);
}

TEST_F(ShortenLinkTest, Execute_WithExpiration_SetsPolicy) {
  ShortenLink::Input input{.original_url = "https://example.com",
                           .expires_after = std::chrono::hours(24)};

  auto result = m_use_case->execute(input);

  ASSERT_TRUE(result.is_ok());
  // Verify the link was saved with expiration
  auto code = ShortCode::from_trusted(result.value().short_code);
  auto found = m_repository->find_by_code(code);
  ASSERT_TRUE(found.is_ok());
  EXPECT_TRUE(found.value().expiration().expires());
}

} // namespace uri_shortener::application::test
