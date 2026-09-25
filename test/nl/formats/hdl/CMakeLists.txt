# SPDX-License-Identifier: Apache-2.0

add_executable(snlRTLPrimitivesTests SNLRTLPrimitivesTest.cpp)
target_link_libraries(snlRTLPrimitivesTests naja_snl_hdl gtest_main)
gtest_discover_tests(snlRTLPrimitivesTests)
