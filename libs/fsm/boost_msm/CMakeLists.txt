# Create interface library (Boost MSM is header-only)
add_library(boost_msm INTERFACE)

target_include_directories(boost_msm INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link against Boost (MSM is header only but might need system/mpl)
find_package(Boost REQUIRED)
target_link_libraries(boost_msm INTERFACE Boost::boost)

# Enable testing
enable_testing()

# Test executable
add_executable(test_boost_msm
    tests/boost_msm_test.cpp
)

target_link_libraries(test_boost_msm
    PRIVATE
        boost_msm
)

# Add tests
add_test(NAME boost_msm_basic_test COMMAND test_boost_msm)
