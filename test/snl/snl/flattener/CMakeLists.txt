include(GoogleTest)

set(snl_flattener_tests
    SNLFlattenerTest0.cpp SNLFlattenerTest1.cpp
    SNLFlattenerTestDelete.cpp
)

add_executable(snlFlattenerTests ${snl_flattener_tests})

target_compile_definitions(snlFlattenerTests PRIVATE FLATTENER_DUMP_PATHS="${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(snlFlattenerTests snl_netlists gmock gtest gtest_main)

GTEST_DISCOVER_TESTS(snlFlattenerTests)