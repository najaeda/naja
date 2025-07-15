# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(snl_vrl_constructor_tests
    SNLVRLConstructorUtilsTest0.cpp
    SNLVRLConstructorTest0.cpp
    SNLVRLConstructorTest1.cpp
    SNLVRLConstructorTestAttributes.cpp
    SNLVRLConstructorTestGate0.cpp
    SNLVRLConstructorTestGate1.cpp
)
add_executable(snlVRLConstructorTests ${snl_vrl_constructor_tests})
target_compile_definitions(snlVRLConstructorTests PRIVATE
  SNL_VRL_BENCHMARKS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../benchmarks"
  SNL_VRL_DUMPER_TEST_PATH="${CMAKE_CURRENT_BINARY_DIR}"
)
target_link_libraries(snlVRLConstructorTests naja_snl_verilog gmock gtest_main)
GTEST_DISCOVER_TESTS(snlVRLConstructorTests
  DISCOVERY_TIMEOUT 30
  PROPERTIES ENVIRONMENT
  PYTHONPATH=${PROJECT_BINARY_DIR}/src/nl/python/naja_wrapping)

if(NOT BUILD_NAJA_PYTHON)
  set(snl_vrl_constructor_tests_with_pyloader
    SNLVRLConstructorTest2.cpp
    SNLVRLConstructorTestDefParams.cpp
    SNLVRLConstructorTestErrors.cpp
  )
  add_executable(snlVRLConstructorPyLoaderTests ${snl_vrl_constructor_tests_with_pyloader})
  target_compile_definitions(snlVRLConstructorPyLoaderTests PRIVATE
    SNL_VRL_BENCHMARKS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../benchmarks")
  target_link_libraries(snlVRLConstructorPyLoaderTests naja_snl_pyloader gmock gtest_main)
  GTEST_DISCOVER_TESTS(snlVRLConstructorPyLoaderTests
    DISCOVERY_TIMEOUT 30
    PROPERTIES ENVIRONMENT
    PYTHONPATH=${PROJECT_BINARY_DIR}/src/nl/python/naja_wrapping)
endif(NOT BUILD_NAJA_PYTHON)
  