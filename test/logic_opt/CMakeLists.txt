# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(sources
  LoadlessLogicRemoverTests.cpp
)

ADD_EXECUTABLE(loadlessLogicRemoverTests ${sources})

TARGET_LINK_LIBRARIES(loadlessLogicRemoverTests naja_dnl naja_opt gmock gtest_main)

GTEST_DISCOVER_TESTS(loadlessLogicRemoverTests)

SET(constantPropagationSources
  ConstantPropagationTests.cpp
)

ADD_EXECUTABLE(constantPropagationTests ${constantPropagationSources})

TARGET_LINK_LIBRARIES(constantPropagationTests naja_opt naja_snl_visual gmock gtest_main)

GTEST_DISCOVER_TESTS(constantPropagationTests)

SET(reductionOptSources
  ReductionOptTests.cpp
)

ADD_EXECUTABLE(reductionOptTests ${reductionOptSources})

target_compile_definitions(reductionOptTests PRIVATE
  SNL_PRIMITIVES_TEST_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

TARGET_LINK_LIBRARIES(reductionOptTests naja_opt naja_snl_pyloader gmock gtest_main)

gtest_discover_tests(
  reductionOptTests
  PROPERTIES ENVIRONMENT
  PYTHONPATH=${PROJECT_BINARY_DIR}/src/snl/python/snl_wrapping
)
