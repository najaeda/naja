include(GoogleTest)

set(snl_fbs_tests
    SNLFBSTest0.cpp
)

add_executable(snlFBSTests ${snl_fbs_tests})

#target_compile_definitions(snlVRLTests PRIVATE
#    SNL_VRL_DUMPER_TEST_PATH="${CMAKE_CURRENT_BINARY_DIR}"
#    SNL_VRL_DUMPER_REFERENCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/references")
target_link_libraries(snlFBSTests naja_snl_fbs gtest gtest_main)

GTEST_DISCOVER_TESTS(snlFBSTests)