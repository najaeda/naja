# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

include(GoogleTest)

set(snl_capnp_tests
    SNLDumpManifestTest.cpp
    SNLCapnPTest0.cpp
    SNLCapnPTest1.cpp
    SNLCapnPStreamingReceiveTest.cpp
)

#add_executable(snlSenderTest SNLSender.cpp)
add_executable(snlCapnPTests ${snl_capnp_tests})


#target_link_libraries(snlSenderTest naja_snl_dump)

target_compile_definitions(snlCapnPTests PRIVATE
    SNL_CAPNP_TEST_PATH="${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(snlCapnPTests naja_snl_dump gtest gmock gtest_main)

GTEST_DISCOVER_TESTS(snlCapnPTests)