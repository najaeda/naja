# SPDX-License-Identifier: Apache-2.0

add_executable(snlVHDLConstructorTests VHDLConstructorTest.cpp)
target_compile_definitions(snlVHDLConstructorTests PRIVATE
  SNL_VHDL_VARIABLES="${CMAKE_CURRENT_SOURCE_DIR}/variables.vhd"
  SNL_VHDL_PIPELINE="${CMAKE_CURRENT_SOURCE_DIR}/pipeline.vhd"
  SNL_VHDL_EQUIVALENT_MUX_SV="${CMAKE_CURRENT_SOURCE_DIR}/mux_equivalent.sv"
  SNL_VHDL_EQUIVALENT_REGISTER_SV="${CMAKE_CURRENT_SOURCE_DIR}/register_equivalent.sv")
target_link_libraries(snlVHDLConstructorTests
  naja_snl_vhdl
  naja_snl_systemverilog
  gtest_main)
gtest_discover_tests(snlVHDLConstructorTests)

find_program(VHDL_NVC_EXECUTABLE nvc)
find_package(Python3 COMPONENTS Interpreter QUIET)
if(VHDL_NVC_EXECUTABLE AND Python3_Interpreter_FOUND)
  add_test(NAME VHDLPipelineReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_pipeline.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLVariableReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_variables.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  set_tests_properties(VHDLPipelineReference VHDLVariableReference PROPERTIES TIMEOUT 180)
endif()
