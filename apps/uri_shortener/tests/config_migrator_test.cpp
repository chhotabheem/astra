/// @file config_migrator_test.cpp
/// @brief TDD tests for ConfigMigrator - schema migration following Protobuf best practices

#include <gtest/gtest.h>
#include "config/ConfigMigrator.h"

namespace uri_shortener::test {

// =============================================================================
// CURRENT VERSION TESTS
// =============================================================================

TEST(ConfigMigratorTest, CurrentVersionNoMigration) {
    Config config;
    config.set_schema_version(CURRENT_SCHEMA_VERSION);
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    auto result = ConfigMigrator::migrate(std::move(config));
    
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.config.schema_version(), CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(result.config.bootstrap().server().port(), 8080);
    EXPECT_FALSE(result.migration_applied);  // No transformation needed
}

TEST(ConfigMigratorTest, CurrentVersionReturned) {
    EXPECT_EQ(ConfigMigrator::currentVersion(), CURRENT_SCHEMA_VERSION);
}

// =============================================================================
// FORWARD COMPATIBILITY TESTS (Future versions)
// =============================================================================

TEST(ConfigMigratorTest, FutureVersionAccepted) {
    Config config;
    config.set_schema_version(CURRENT_SCHEMA_VERSION + 10);  // Future version
    config.mutable_bootstrap()->mutable_server()->set_port(9000);
    
    auto result = ConfigMigrator::migrate(std::move(config));
    
    // Should accept future versions (Protobuf ignores unknown fields)
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.config.bootstrap().server().port(), 9000);
    EXPECT_FALSE(result.migration_applied);  // No transformation
}

TEST(ConfigMigratorTest, FutureVersionPreserved) {
    Config config;
    config.set_schema_version(99);  // Way in the future
    
    auto result = ConfigMigrator::migrate(std::move(config));
    
    ASSERT_TRUE(result.success);
    // Version preserved for future compatibility
    EXPECT_EQ(result.config.schema_version(), 99);
}

// =============================================================================
// BACKWARD COMPATIBILITY TESTS (Old versions)
// =============================================================================

TEST(ConfigMigratorTest, OldVersionMigrated) {
    Config config;
    config.set_schema_version(1);  // Older version (when we're on v2+)
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    auto result = ConfigMigrator::migrate(std::move(config));
    
    ASSERT_TRUE(result.success);
    // Data preserved
    EXPECT_EQ(result.config.bootstrap().server().port(), 8080);
}

TEST(ConfigMigratorTest, ZeroVersionTreatedAsV1) {
    Config config;
    config.set_schema_version(0);  // Missing/default version
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    
    auto result = ConfigMigrator::migrate(std::move(config));
    
    ASSERT_TRUE(result.success);
    EXPECT_GE(result.config.schema_version(), 1);  // Should be at least v1
    EXPECT_EQ(result.config.bootstrap().server().port(), 8080);
}

TEST(ConfigMigratorTest, NegativeVersionTreatedAsV1) {
    Config config;
    config.set_schema_version(-5);
    
    auto result = ConfigMigrator::migrate(std::move(config));
    
    ASSERT_TRUE(result.success);
    EXPECT_GE(result.config.schema_version(), 1);
}

// =============================================================================
// DATA PRESERVATION TESTS
// =============================================================================

TEST(ConfigMigratorTest, PreservesAllFields) {
    Config config;
    config.set_schema_version(CURRENT_SCHEMA_VERSION);
    config.mutable_bootstrap()->mutable_server()->set_address("0.0.0.0");
    config.mutable_bootstrap()->mutable_server()->set_port(8080);
    config.mutable_bootstrap()->mutable_threading()->set_worker_threads(4);
    config.mutable_operational()->mutable_logging()->set_level("INFO");
    config.mutable_runtime()->mutable_rate_limiting()->set_global_rps_limit(100000);
    
    auto result = ConfigMigrator::migrate(std::move(config));
    
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.config.bootstrap().server().address(), "0.0.0.0");
    EXPECT_EQ(result.config.bootstrap().server().port(), 8080);
    EXPECT_EQ(result.config.bootstrap().threading().worker_threads(), 4);
    EXPECT_EQ(result.config.operational().logging().level(), "INFO");
    EXPECT_EQ(result.config.runtime().rate_limiting().global_rps_limit(), 100000);
}

TEST(ConfigMigratorTest, PreservesNestedMessages) {
    Config config;
    config.set_schema_version(CURRENT_SCHEMA_VERSION);
    config.mutable_operational()->mutable_observability()->set_metrics_enabled(true);
    config.mutable_operational()->mutable_observability()->set_otlp_endpoint("http://otel:4317");
    config.mutable_runtime()->mutable_feature_flags()->set_enable_caching(true);
    
    auto result = ConfigMigrator::migrate(std::move(config));
    
    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.config.operational().observability().metrics_enabled());
    EXPECT_EQ(result.config.operational().observability().otlp_endpoint(), "http://otel:4317");
    EXPECT_TRUE(result.config.runtime().feature_flags().enable_caching());
}

// =============================================================================
// MIGRATION RESULT TESTS
// =============================================================================

TEST(ConfigMigratorTest, ResultContainsFromVersion) {
    Config config;
    config.set_schema_version(CURRENT_SCHEMA_VERSION);
    
    auto result = ConfigMigrator::migrate(std::move(config));
    
    EXPECT_EQ(result.from_version, CURRENT_SCHEMA_VERSION);
    EXPECT_EQ(result.to_version, CURRENT_SCHEMA_VERSION);
}

TEST(ConfigMigratorTest, NeedsMigrationCheck) {
    Config current;
    current.set_schema_version(CURRENT_SCHEMA_VERSION);
    EXPECT_FALSE(ConfigMigrator::needsTransformation(current.schema_version()));
    
    // When we add v2, old versions will need transformation
    // For now, Protobuf handles defaults automatically
}

// =============================================================================
// INTEGRATION WITH PROTOLOADER
// =============================================================================

TEST(ConfigMigratorIntegrationTest, LoaderAutoMigrates) {
    // This is tested in proto_config_loader_test.cpp
    // Here we just verify the API works
    Config config;
    config.set_schema_version(0);  // Legacy
    
    auto result = ConfigMigrator::migrate(std::move(config));
    EXPECT_TRUE(result.success);
}

} // namespace uri_shortener::test
