# SPDX-License-Identifier: Apache-2.0

add_executable(snlVHDLConstructorTests VHDLConstructorTest.cpp)
target_compile_definitions(snlVHDLConstructorTests PRIVATE
  SNL_VHDL_EQUIVALENT_SV="${CMAKE_CURRENT_SOURCE_DIR}/mux_equivalent.sv")
target_link_libraries(snlVHDLConstructorTests
  naja_snl_vhdl
  naja_snl_systemverilog
  gtest_main)
gtest_discover_tests(snlVHDLConstructorTests)
