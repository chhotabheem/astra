#include <gtest/gtest.h>
#include "ConfigValidator.h"
#include "ConfigStructs.h"

namespace config {

TEST(ConfigValidatorTest, ValidBootstrapConfigPasses) {
    BootstrapConfig bootstrap;
    bootstrap.m_server.m_port = 8080;
    bootstrap.m_threading.m_worker_threads = 2;
    bootstrap.m_database.m_mongodb_uri = "mongodb://localhost:27017";
    bootstrap.m_database.m_redis_uri = "redis://localhost:6379";
    
    EXPECT_NO_THROW(ConfigValidator::validateBootstrap(bootstrap));
}

TEST(ConfigValidatorTest, InvalidPortZeroThrows) {
    BootstrapConfig bootstrap;
    bootstrap.m_server.m_port = 0;
    bootstrap.m_database.m_mongodb_uri = "mongodb://localhost:27017";
    bootstrap.m_database.m_redis_uri = "redis://localhost:6379";
    
    EXPECT_THROW(ConfigValidator::validateBootstrap(bootstrap), std::invalid_argument);
}

TEST(ConfigValidatorTest, ZeroWorkerThreadsThrows) {
    BootstrapConfig bootstrap;
    bootstrap.m_threading.m_worker_threads = 0;
    bootstrap.m_database.m_mongodb_uri = "mongodb://localhost:27017";
    bootstrap.m_database.m_redis_uri = "redis://localhost:6379";
    
    EXPECT_THROW(ConfigValidator::validateBootstrap(bootstrap), std::invalid_argument);
}

TEST(ConfigValidatorTest, EmptyMongoUriThrows) {
    BootstrapConfig bootstrap;
    bootstrap.m_database.m_mongodb_uri = "";
    
    EXPECT_THROW(ConfigValidator::validateBootstrap(bootstrap), std::invalid_argument);
}

TEST(ConfigValidatorTest, ValidOperationalConfigPasses) {
    OperationalConfig operational;
    operational.m_logging.m_level = "INFO";
    operational.m_timeouts.m_request_ms = 5000;
    operational.m_connection_pools.m_mongodb_pool_size = 10;
    
    auto error = ConfigValidator::validateOperational(operational);
    EXPECT_FALSE(error.has_value());
}

TEST(ConfigValidatorTest, InvalidLogLevelReturnsError) {
    OperationalConfig operational;
    operational.m_logging.m_level = "INVALID_LEVEL";
    
    auto error = ConfigValidator::validateOperational(operational);
    EXPECT_TRUE(error.has_value());
    EXPECT_NE(error->find("log level"), std::string::npos);
}

TEST(ConfigValidatorTest, NegativeTimeoutReturnsError) {
    OperationalConfig operational;
    operational.m_timeouts.m_request_ms = -100;
    
    auto error = ConfigValidator::validateOperational(operational);
    EXPECT_TRUE(error.has_value());
    EXPECT_NE(error->find("timeout"), std::string::npos);
}

TEST(ConfigValidatorTest, ZeroPoolSizeReturnsError) {
    OperationalConfig operational;
    operational.m_connection_pools.m_mongodb_pool_size = 0;
    
    auto error = ConfigValidator::validateOperational(operational);
    EXPECT_TRUE(error.has_value());
    EXPECT_NE(error->find("pool size"), std::string::npos);
}

TEST(ConfigValidatorTest, ValidRuntimeConfigPasses) {
    RuntimeConfig runtime;
    runtime.m_rate_limiting.m_global_rps_limit = 100000;
    runtime.m_circuit_breaker.m_mongodb_threshold = 5;
    
    auto error = ConfigValidator::validateRuntime(runtime);
    EXPECT_FALSE(error.has_value());
}

TEST(ConfigValidatorTest, NegativeRpsLimitReturnsError) {
    RuntimeConfig runtime;
    runtime.m_rate_limiting.m_global_rps_limit = -1;
    
    auto error = ConfigValidator::validateRuntime(runtime);
    EXPECT_TRUE(error.has_value());
    EXPECT_NE(error->find("rate limit"), std::string::npos);
}

TEST(ConfigValidatorTest, ZeroCircuitBreakerThresholdReturnsError) {
    RuntimeConfig runtime;
    runtime.m_circuit_breaker.m_mongodb_threshold = 0;
    
    auto error = ConfigValidator::validateRuntime(runtime);
    EXPECT_TRUE(error.has_value());
    EXPECT_NE(error->find("Circuit breaker"), std::string::npos);
}

TEST(ConfigValidatorTest, ValidCompleteConfigPasses) {
    Config config;
    config.m_bootstrap.m_database.m_mongodb_uri = "mongodb://localhost:27017";
    config.m_bootstrap.m_database.m_redis_uri = "redis://localhost:6379";
    config.m_operational.m_logging.m_level = "INFO";
    config.m_runtime.m_rate_limiting.m_global_rps_limit = 100000;
    
    EXPECT_NO_THROW(ConfigValidator::validate(config));
}

TEST(ConfigValidatorTest, AllValidLogLevelsAccepted) {
    OperationalConfig operational;
    
    for (const auto& level : {"DEBUG", "INFO", "WARN", "ERROR"}) {
        operational.m_logging.m_level = level;
        auto error = ConfigValidator::validateOperational(operational);
        EXPECT_FALSE(error.has_value()) << "Level " << level << " should be valid";
    }
}

}
