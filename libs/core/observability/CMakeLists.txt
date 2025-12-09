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
    src/Context.cpp
    src/Observability.cpp
    src/otel/OTelBackend.cpp
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
    test/context_test.cpp
    test/span_test.cpp
    test/log_test.cpp
    test/metrics_test.cpp
    test/thread_safety_test.cpp
    test/console_backend_test.cpp
    test/seda_tracing_test.cpp
)

target_link_libraries(test_observability
    PRIVATE
        observability
        astra_execution
        GTest::gtest_main
)

add_test(NAME observability_tests COMMAND test_observability)

