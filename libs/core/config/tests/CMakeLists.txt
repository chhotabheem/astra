add_executable(config_structs_test config_structs_test.cpp)
target_link_libraries(config_structs_test PRIVATE GTest::gtest GTest::gtest_main)
target_include_directories(config_structs_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(config_source_test config_source_test.cpp)
target_link_libraries(config_source_test PRIVATE GTest::gtest GTest::gtest_main)
target_include_directories(config_source_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(config_validator_test config_validator_test.cpp)
target_link_libraries(config_validator_test PRIVATE GTest::gtest GTest::gtest_main config_impl)
target_include_directories(config_validator_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(file_config_source_test file_config_source_test.cpp)
target_link_libraries(file_config_source_test PRIVATE GTest::gtest GTest::gtest_main config_impl)
target_include_directories(file_config_source_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(json_config_parser_test json_config_parser_test.cpp)
target_link_libraries(json_config_parser_test PRIVATE GTest::gtest GTest::gtest_main config_impl)
target_include_directories(json_config_parser_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_definitions(json_config_parser_test PRIVATE CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

add_executable(config_provider_test config_provider_test.cpp)
target_link_libraries(config_provider_test PRIVATE GTest::gtest GTest::gtest_main config_impl)
target_include_directories(config_provider_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(config_provider_di_test config_provider_di_test.cpp)
target_link_libraries(config_provider_di_test PRIVATE GTest::gtest GTest::gtest_main config_impl)
target_include_directories(config_provider_di_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(config_edge_cases_test config_edge_cases_test.cpp)
target_link_libraries(config_edge_cases_test PRIVATE GTest::gtest GTest::gtest_main config_impl)
target_include_directories(config_edge_cases_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(inotify_file_watcher_test inotify_file_watcher_test.cpp)
target_link_libraries(inotify_file_watcher_test PRIVATE GTest::gtest GTest::gtest_main config_impl)
target_include_directories(inotify_file_watcher_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

gtest_discover_tests(config_structs_test)
gtest_discover_tests(config_source_test)
gtest_discover_tests(config_validator_test)
gtest_discover_tests(file_config_source_test)
gtest_discover_tests(json_config_parser_test)
gtest_discover_tests(config_provider_test)
gtest_discover_tests(config_provider_di_test)
gtest_discover_tests(config_edge_cases_test)
gtest_discover_tests(inotify_file_watcher_test)
