/// @file test_observable_repository.cpp
/// @brief Tests for ObservableLinkRepository decorator

#include "InMemoryLinkRepository.h"
#include "ObservableLinkRepository.h"
#include "ShortLink.h"

#include <Provider.h>
#include <gtest/gtest.h>

namespace uri_shortener::test {

class ObservableLinkRepositoryTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Initialize observability with Provider pattern
    ::observability::Config obs_config;
    obs_config.set_service_name("uri_shortener_test");
    obs_config.set_service_version("1.0.0");
    obs_config.set_environment("test");
    obs::init(obs_config);

    m_inner = std::make_shared<infrastructure::InMemoryLinkRepository>();
    m_repo =
        std::make_shared<infrastructure::ObservableLinkRepository>(m_inner);
  }

  void TearDown() override {
    obs::shutdown();
  }

  std::shared_ptr<infrastructure::InMemoryLinkRepository> m_inner;
  std::shared_ptr<infrastructure::ObservableLinkRepository> m_repo;
};

TEST_F(ObservableLinkRepositoryTest, Save_DelegatesAndReturnsSuccess) {
  auto code = domain::ShortCode::create("abc123").value();
  auto url = domain::OriginalUrl::create("https://example.com").value();
  auto link = domain::ShortLink::create(code, url).value();

  auto result = m_repo->save(link);

  EXPECT_TRUE(result.is_ok());
  EXPECT_TRUE(m_inner->exists(code));
}

TEST_F(ObservableLinkRepositoryTest, FindByCode_DelegatesAndReturnsResult) {
  auto code = domain::ShortCode::create("abc123").value();
  auto url = domain::OriginalUrl::create("https://example.com").value();
  auto link = domain::ShortLink::create(code, url).value();
  m_inner->save(link);

  auto result = m_repo->find_by_code(code);

  EXPECT_TRUE(result.is_ok());
  EXPECT_EQ(result.value().code().value(), "abc123");
}

TEST_F(ObservableLinkRepositoryTest, FindByCode_NotFound_ReturnsError) {
  auto code = domain::ShortCode::create("notfnd").value();

  auto result = m_repo->find_by_code(code);

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.error(), domain::DomainError::LinkNotFound);
}

TEST_F(ObservableLinkRepositoryTest, Remove_DelegatesAndRemovesFromInner) {
  auto code = domain::ShortCode::create("abc123").value();
  auto url = domain::OriginalUrl::create("https://example.com").value();
  auto link = domain::ShortLink::create(code, url).value();
  m_inner->save(link);

  auto result = m_repo->remove(code);

  EXPECT_TRUE(result.is_ok());
  EXPECT_FALSE(m_inner->exists(code));
}

TEST_F(ObservableLinkRepositoryTest, Exists_DelegatesCorrectly) {
  auto code = domain::ShortCode::create("abc123").value();
  auto url = domain::OriginalUrl::create("https://example.com").value();
  auto link = domain::ShortLink::create(code, url).value();
  m_inner->save(link);

  EXPECT_TRUE(m_repo->exists(code));

  auto missing = domain::ShortCode::create("notfnd").value();
  EXPECT_FALSE(m_repo->exists(missing));
}

} // namespace uri_shortener::test
