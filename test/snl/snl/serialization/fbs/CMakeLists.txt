include(GoogleTest)

set(snl_fbs_tests
    SNLFBSTest0.cpp
)

add_executable(snlFBSTests ${snl_fbs_tests})

target_compile_definitions(snlFBSTests PRIVATE
    SNL_FBS_TEST_PATH="${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(snlFBSTests naja_snl_fbs gtest gtest_main)

GTEST_DISCOVER_TESTS(snlFBSTests)