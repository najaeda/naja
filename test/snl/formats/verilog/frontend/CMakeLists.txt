# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(snl_vrl_constructor_tests
  SNLVRLConstructorUtilsTest0.cpp
  SNLVRLConstructorTest0.cpp
  SNLVRLConstructorTest1.cpp
  SNLVRLConstructorTestAttributes.cpp
)

set(snl_vrl_constructor_pyloader_tests
  SNLVRLConstructorTest2.cpp
  SNLVRLConstructorTestDefParams.cpp
  SNLVRLConstructorTestErrors.cpp
)

if(BUILD_NAJA_PYTHON)
  add_executable(snlVRLConstructorTests ${snl_vrl_constructor_tests})
  target_link_libraries(snlVRLConstructorTests naja_snl_verilog gmock gtest_main)
else(BUILD_NAJA_PYTHON)
  add_executable(
    snlVRLConstructorTests
    ${snl_vrl_constructor_tests}
    ${snl_vrl_constructor_pyloader_tests})
  target_link_libraries(snlVRLConstructorTests naja_snl_verilog naja_snl_pyloader gmock gtest_main)
endif(BUILD_NAJA_PYTHON)

target_compile_definitions(snlVRLConstructorTests PRIVATE
  SNL_VRL_BENCHMARKS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../benchmarks")

GTEST_DISCOVER_TESTS(snlVRLConstructorTests
  PROPERTIES ENVIRONMENT
  PYTHONPATH=${PROJECT_BINARY_DIR}/src/snl/python/snl_wrapping)