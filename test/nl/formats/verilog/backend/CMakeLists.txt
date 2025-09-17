# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

include(GoogleTest)

set(snl_vrl_dumper_tests
    SNLVRLDumperTest0.cpp
    SNLVRLDumperTest1.cpp
    SNLVRLDumperTest2.cpp
    SNLVRLDumperTest3.cpp
    SNLVRLDumperTest4.cpp
    SNLVRLDumperTestEscaping.cpp
    SNLVRLDumperTestTermNets.cpp
    SNLVRLDumperTestParameters.cpp
    SNLVRLDumperTestAttributes.cpp
    SNLVRLDumperTestUtils.cpp
    SNLVRLDumperTestLibraryDump.cpp
    SNLVRLDumperTestGate0.cpp
)

add_executable(snlVRLDumperTests ${snl_vrl_dumper_tests})

target_compile_definitions(snlVRLDumperTests PRIVATE
    SNL_VRL_DUMPER_TEST_PATH="${CMAKE_CURRENT_BINARY_DIR}"
    SNL_VRL_DUMPER_REFERENCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/references"
    NAJA_DIFF="${CMAKE_SOURCE_DIR}/test/test_utils/diff_files.py")
target_link_libraries(snlVRLDumperTests naja_snl_verilog gtest_main)

GTEST_DISCOVER_TESTS(snlVRLDumperTests)