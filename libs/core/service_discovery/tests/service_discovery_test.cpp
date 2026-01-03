#include "IServiceResolver.h"
#include "StaticServiceResolver.h"

#include <gtest/gtest.h>
#include <string>

using namespace astra::service_discovery;

// =============================================================================
// IServiceResolver Interface Tests (via StaticServiceResolver)
// =============================================================================

class StaticServiceResolverTest : public ::testing::Test {};

// Happy path: resolve returns registered service
TEST_F(StaticServiceResolverTest, ResolveReturnsRegisteredService) {
  StaticServiceResolver resolver;
  resolver.register_service("dataservice", "localhost", 8080);

  auto [host, port] = resolver.resolve("dataservice");

  EXPECT_EQ(host, "localhost");
  EXPECT_EQ(port, 8080);
}

// Multiple services can be registered and resolved
TEST_F(StaticServiceResolverTest, ResolveMultipleServices) {
  StaticServiceResolver resolver;
  resolver.register_service("service-a", "host-a.local", 9001);
  resolver.register_service("service-b", "host-b.local", 9002);
  resolver.register_service("service-c", "host-c.local", 9003);

  auto [host_a, port_a] = resolver.resolve("service-a");
  auto [host_b, port_b] = resolver.resolve("service-b");
  auto [host_c, port_c] = resolver.resolve("service-c");

  EXPECT_EQ(host_a, "host-a.local");
  EXPECT_EQ(port_a, 9001);
  EXPECT_EQ(host_b, "host-b.local");
  EXPECT_EQ(port_b, 9002);
  EXPECT_EQ(host_c, "host-c.local");
  EXPECT_EQ(port_c, 9003);
}

// Overwriting a service registration updates the address
TEST_F(StaticServiceResolverTest, RegisterOverwritesPrevious) {
  StaticServiceResolver resolver;
  resolver.register_service("dataservice", "old-host", 1111);
  resolver.register_service("dataservice", "new-host", 2222);

  auto [host, port] = resolver.resolve("dataservice");

  EXPECT_EQ(host, "new-host");
  EXPECT_EQ(port, 2222);
}

// =============================================================================
// Edge Cases
// =============================================================================

// Resolve unknown service throws exception
TEST_F(StaticServiceResolverTest, ResolveUnknownServiceThrows) {
  StaticServiceResolver resolver;

  EXPECT_THROW({ resolver.resolve("unknown-service"); }, std::runtime_error);
}

// Empty service name throws on registration
TEST_F(StaticServiceResolverTest, RegisterEmptyServiceNameThrows) {
  StaticServiceResolver resolver;

  EXPECT_THROW(
      { resolver.register_service("", "host", 8080); }, std::invalid_argument);
}

// Empty host throws on registration
TEST_F(StaticServiceResolverTest, RegisterEmptyHostThrows) {
  StaticServiceResolver resolver;

  EXPECT_THROW(
      { resolver.register_service("service", "", 8080); },
      std::invalid_argument);
}

// Port 0 is allowed (ephemeral port)
TEST_F(StaticServiceResolverTest, RegisterPortZeroAllowed) {
  StaticServiceResolver resolver;

  EXPECT_NO_THROW({ resolver.register_service("ephemeral", "localhost", 0); });

  auto [host, port] = resolver.resolve("ephemeral");
  EXPECT_EQ(port, 0);
}

// =============================================================================
// Service Names with Special Characters
// =============================================================================

TEST_F(StaticServiceResolverTest, ServiceNameWithDashes) {
  StaticServiceResolver resolver;
  resolver.register_service("my-data-service", "host", 8080);

  auto [host, port] = resolver.resolve("my-data-service");
  EXPECT_EQ(host, "host");
}

TEST_F(StaticServiceResolverTest, ServiceNameWithUnderscores) {
  StaticServiceResolver resolver;
  resolver.register_service("my_data_service", "host", 8080);

  auto [host, port] = resolver.resolve("my_data_service");
  EXPECT_EQ(host, "host");
}

TEST_F(StaticServiceResolverTest, ServiceNameWithDots) {
  StaticServiceResolver resolver;
  resolver.register_service("com.example.service", "host", 8080);

  auto [host, port] = resolver.resolve("com.example.service");
  EXPECT_EQ(host, "host");
}

// =============================================================================
// Host Formats
// =============================================================================

TEST_F(StaticServiceResolverTest, IPv4Address) {
  StaticServiceResolver resolver;
  resolver.register_service("svc", "192.168.1.100", 8080);

  auto [host, port] = resolver.resolve("svc");
  EXPECT_EQ(host, "192.168.1.100");
}

TEST_F(StaticServiceResolverTest, IPv6Address) {
  StaticServiceResolver resolver;
  resolver.register_service("svc", "::1", 8080);

  auto [host, port] = resolver.resolve("svc");
  EXPECT_EQ(host, "::1");
}

TEST_F(StaticServiceResolverTest, FullyQualifiedDomainName) {
  StaticServiceResolver resolver;
  resolver.register_service("svc", "api.example.com", 443);

  auto [host, port] = resolver.resolve("svc");
  EXPECT_EQ(host, "api.example.com");
}

// =============================================================================
// Port Edge Cases
// =============================================================================

TEST_F(StaticServiceResolverTest, HighPort) {
  StaticServiceResolver resolver;
  resolver.register_service("svc", "host", 65535);

  auto [host, port] = resolver.resolve("svc");
  EXPECT_EQ(port, 65535);
}

TEST_F(StaticServiceResolverTest, CommonPorts) {
  StaticServiceResolver resolver;
  resolver.register_service("http", "host", 80);
  resolver.register_service("https", "host", 443);
  resolver.register_service("grpc", "host", 50051);

  EXPECT_EQ(resolver.resolve("http").second, 80);
  EXPECT_EQ(resolver.resolve("https").second, 443);
  EXPECT_EQ(resolver.resolve("grpc").second, 50051);
}

// =============================================================================
// Utility Methods
// =============================================================================

TEST_F(StaticServiceResolverTest, HasServiceReturnsTrue) {
  StaticServiceResolver resolver;
  resolver.register_service("existing", "host", 8080);

  EXPECT_TRUE(resolver.has_service("existing"));
}

TEST_F(StaticServiceResolverTest, HasServiceReturnsFalse) {
  StaticServiceResolver resolver;

  EXPECT_FALSE(resolver.has_service("nonexistent"));
}

TEST_F(StaticServiceResolverTest, UnregisterRemovesService) {
  StaticServiceResolver resolver;
  resolver.register_service("temp", "host", 8080);

  EXPECT_TRUE(resolver.has_service("temp"));

  resolver.unregister_service("temp");

  EXPECT_FALSE(resolver.has_service("temp"));
  EXPECT_THROW(resolver.resolve("temp"), std::runtime_error);
}

TEST_F(StaticServiceResolverTest, UnregisterNonexistentIsNoOp) {
  StaticServiceResolver resolver;

  EXPECT_NO_THROW({ resolver.unregister_service("nonexistent"); });
}
