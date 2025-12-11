# Config library tests - only FileConfigSource and inotify tests remain
# Proto config tests are in apps/uri_shortener/tests/

add_executable(config_source_test config_source_test.cpp)
target_link_libraries(config_source_test PRIVATE GTest::gtest GTest::gtest_main)
target_include_directories(config_source_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(file_config_source_test file_config_source_test.cpp)
target_link_libraries(file_config_source_test PRIVATE GTest::gtest GTest::gtest_main config_impl)
target_include_directories(file_config_source_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(inotify_file_watcher_test inotify_file_watcher_test.cpp)
target_link_libraries(inotify_file_watcher_test PRIVATE GTest::gtest GTest::gtest_main config_impl)
target_include_directories(inotify_file_watcher_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

gtest_discover_tests(config_source_test)
gtest_discover_tests(file_config_source_test)
gtest_discover_tests(inotify_file_watcher_test)
