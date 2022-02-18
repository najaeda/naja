include(GoogleTest)

set(snl_dump_tests
  SNLDumpTest0.cpp
)

add_executable(snlDumpTests ${snl_dump_tests})
target_compile_definitions(snlDumpTests PRIVATE SNL_DUMP_PATHS="${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(snlDumpTests snl_netlists gtest gmock gtest_main)

GTEST_DISCOVER_TESTS(snlDumpTests)