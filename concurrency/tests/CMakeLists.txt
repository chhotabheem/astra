cmake_minimum_required(VERSION 3.25)

add_executable(test_worker_pool test_worker_pool.cpp)

target_link_libraries(test_worker_pool
    PRIVATE
        astra_concurrency
)

add_test(NAME worker_pool_test COMMAND test_worker_pool)
