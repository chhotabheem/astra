add_executable(test_router test_router.cpp)
target_link_libraries(test_router
    PRIVATE
        astra_router
)
add_test(NAME router_test COMMAND test_router)
