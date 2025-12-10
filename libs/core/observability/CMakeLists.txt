# -----------------------------------------------------------------------------
# Find pre-installed OpenTelemetry C++ SDK
# -----------------------------------------------------------------------------
find_package(opentelemetry-cpp CONFIG REQUIRED)
message(STATUS "Found OpenTelemetry C++ SDK: ${opentelemetry-cpp_VERSION}")

# -----------------------------------------------------------------------------
# Observability Library (C++17)
# Static because OpenTelemetry static libs weren't built with -fPIC
# -----------------------------------------------------------------------------
add_library(observability STATIC
    src/Provider.cpp
    src/ProviderImpl.cpp
    src/Metrics.cpp
    src/MetricsRegistry.cpp
    src/Context.cpp
    src/Span.cpp
    src/Log.cpp
)

target_include_directories(observability
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link OTel (PRIVATE - consumers don't see OTel headers)
target_link_libraries(observability
    PRIVATE
        astra_sanitizers
        opentelemetry-cpp::api
        opentelemetry-cpp::sdk
        opentelemetry-cpp::logs
        opentelemetry-cpp::metrics
        opentelemetry-cpp::trace
        opentelemetry-cpp::ostream_span_exporter
        opentelemetry-cpp::ostream_metrics_exporter
        opentelemetry-cpp::ostream_log_record_exporter
        opentelemetry-cpp::resources
)

# Our code is C++17
target_compile_features(observability PUBLIC cxx_std_17)

# -----------------------------------------------------------------------------
# Unit Tests
# -----------------------------------------------------------------------------
add_executable(test_observability
    test/provider_test.cpp
    test/metrics_test.cpp
    test/attributes_test.cpp
    test/metrics_registry_test.cpp
    test/safety_test.cpp
    test/span_test.cpp
    test/log_test.cpp
    test/logging_test.cpp
    test/tracing_test.cpp
    test/integration_test.cpp
    # Extended test suites
    test/provider_extended_test.cpp
    test/metrics_extended_test.cpp
    test/span_extended_test.cpp
    test/context_extended_test.cpp
    test/log_extended_test.cpp
    test/thread_safety_extended_test.cpp
    test/integration_extended_test.cpp
)

target_link_libraries(test_observability
    PRIVATE
        observability
        astra_execution
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_observability)

