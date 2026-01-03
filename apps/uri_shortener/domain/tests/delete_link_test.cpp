/// @file test_delete_link.cpp
/// @brief TDD tests for DeleteLink use case

#include "DeleteLink.h"
#include "ICodeGenerator.h"
#include "ILinkRepository.h"
#include "ShortenLink.h"

#include <gtest/gtest.h>
#include <map>
#include <memory>

namespace uri_shortener::application::test {

using namespace uri_shortener::domain;

class MockCodeGenerator : public ICodeGenerator {
public:
  ShortCode generate() override {
    return ShortCode::from_trusted("del" + std::to_string(m_counter++));
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

  size_t size() const {
    return m_links.size();
  }

private:
  std::map<std::string, ShortLink> m_links;
};

class DeleteLinkTest : public ::testing::Test {
protected:
  void SetUp() override {
    m_repository = std::make_shared<MockLinkRepository>();
    m_generator = std::make_shared<MockCodeGenerator>();
    m_shorten = std::make_unique<ShortenLink>(m_repository, m_generator);
    m_delete = std::make_unique<DeleteLink>(m_repository);
  }

  std::shared_ptr<MockLinkRepository> m_repository;
  std::shared_ptr<MockCodeGenerator> m_generator;
  std::unique_ptr<ShortenLink> m_shorten;
  std::unique_ptr<DeleteLink> m_delete;
};

TEST_F(DeleteLinkTest, Execute_WithExistingCode_RemovesLink) {
  // First create a link
  auto shorten_result =
      m_shorten->execute({.original_url = "https://example.com"});
  ASSERT_TRUE(shorten_result.is_ok());
  auto short_code = shorten_result.value().short_code;
  EXPECT_EQ(m_repository->size(), 1);

  // Now delete it
  auto delete_result = m_delete->execute({.short_code = short_code});

  ASSERT_TRUE(delete_result.is_ok());
  EXPECT_EQ(m_repository->size(), 0);
}

TEST_F(DeleteLinkTest, Execute_WithNonExistentCode_ReturnsNotFound) {
  DeleteLink::Input input{.short_code = "abc123"};

  auto result = m_delete->execute(input);

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.error(), DomainError::LinkNotFound);
}

TEST_F(DeleteLinkTest, Execute_WithInvalidCode_ReturnsError) {
  DeleteLink::Input input{.short_code = "ab"}; // Too short

  auto result = m_delete->execute(input);

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.error(), DomainError::InvalidShortCode);
}

} // namespace uri_shortener::application::test
