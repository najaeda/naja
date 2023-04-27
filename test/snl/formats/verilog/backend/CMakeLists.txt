include(GoogleTest)

set(snl_vrl_dumper_tests
    SNLVRLDumperTest0.cpp
    SNLVRLDumperTest1.cpp
    SNLVRLDumperTest2.cpp
    SNLVRLDumperTestTermNets.cpp
    SNLVRLDumperTestParameters.cpp
    SNLVRLDumperTestUtils.cpp
)

add_executable(snlVRLDumperTests ${snl_vrl_dumper_tests})

target_compile_definitions(snlVRLDumperTests PRIVATE
    SNL_VRL_DUMPER_TEST_PATH="${CMAKE_CURRENT_BINARY_DIR}"
    SNL_VRL_DUMPER_REFERENCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/references")
target_link_libraries(snlVRLDumperTests naja_snl_verilog gtest gtest_main)

GTEST_DISCOVER_TESTS(snlVRLDumperTests)