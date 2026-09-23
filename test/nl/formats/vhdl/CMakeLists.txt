# SPDX-License-Identifier: Apache-2.0

add_executable(snlVHDLConstructorTests VHDLConstructorTest.cpp)
target_compile_definitions(snlVHDLConstructorTests PRIVATE
  SNL_VHDL_LOGICAL="${CMAKE_CURRENT_SOURCE_DIR}/logical.vhd"
  SNL_VHDL_HIERARCHY="${CMAKE_CURRENT_SOURCE_DIR}/hierarchy.vhd"
  SNL_VHDL_ENABLED="${CMAKE_CURRENT_SOURCE_DIR}/enabled.vhd"
  SNL_VHDL_RESET="${CMAKE_CURRENT_SOURCE_DIR}/reset.vhd"
  SNL_VHDL_RESET_ENABLE="${CMAKE_CURRENT_SOURCE_DIR}/reset_enable.vhd"
  SNL_VHDL_VECTORS="${CMAKE_CURRENT_SOURCE_DIR}/vectors.vhd"
  SNL_VHDL_EQUIVALENT_LOGICAL_SV="${CMAKE_CURRENT_SOURCE_DIR}/logical_equivalent.sv"
  SNL_VHDL_EQUIVALENT_VECTOR_MUX_SV="${CMAKE_CURRENT_SOURCE_DIR}/vector_mux_equivalent.sv"
  SNL_VHDL_RETAINED_VARIABLES="${CMAKE_CURRENT_SOURCE_DIR}/retained_variables.vhd"
  SNL_VHDL_VARIABLES="${CMAKE_CURRENT_SOURCE_DIR}/variables.vhd"
  SNL_VHDL_PIPELINE="${CMAKE_CURRENT_SOURCE_DIR}/pipeline.vhd"
  SNL_VHDL_EQUIVALENT_MUX_SV="${CMAKE_CURRENT_SOURCE_DIR}/mux_equivalent.sv"
  SNL_VHDL_EQUIVALENT_REGISTER_SV="${CMAKE_CURRENT_SOURCE_DIR}/register_equivalent.sv")
target_link_libraries(snlVHDLConstructorTests
  naja_snl_vhdl
  naja_snl_systemverilog
  gtest_main)
gtest_discover_tests(snlVHDLConstructorTests)

# Downloaded inputs are exercised explicitly by vhdl-regress.yml, not CTest.
add_executable(snlVHDLExternalTests EXCLUDE_FROM_ALL VHDLExternalTest.cpp)
target_link_libraries(snlVHDLExternalTests naja_snl_vhdl gtest_main)

find_program(VHDL_NVC_EXECUTABLE nvc)
find_package(Python3 COMPONENTS Interpreter QUIET)
if(VHDL_NVC_EXECUTABLE AND Python3_Interpreter_FOUND)
  add_test(NAME VHDLPipelineReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_pipeline.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLVariableReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_variables.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLRetainedVariableReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_retained_variables.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLLogicalReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_logical.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLVectorReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_vectors.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLHierarchyReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_hierarchy.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLEnableReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_enable.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLResetReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_reset.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  add_test(NAME VHDLResetEnableReference
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/compare_reset_enable.py"
      --nvc "${VHDL_NVC_EXECUTABLE}" --adapter $<TARGET_FILE:snlVHDLConstructorTests>)
  set_tests_properties(VHDLPipelineReference VHDLVariableReference
    VHDLRetainedVariableReference VHDLLogicalReference VHDLVectorReference
    VHDLHierarchyReference VHDLEnableReference VHDLResetReference
    VHDLResetEnableReference
    PROPERTIES TIMEOUT 180)
endif()
