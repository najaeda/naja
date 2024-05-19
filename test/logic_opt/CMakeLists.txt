# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(sources
  LoadlessLogicRemoverTests.cpp
)

ADD_EXECUTABLE(LoadlessLogicRemoverTests ${sources})

TARGET_LINK_LIBRARIES(LoadlessLogicRemoverTests naja_dnl naja_opt gmock gtest_main)

GTEST_DISCOVER_TESTS(LoadlessLogicRemoverTests)

SET(constantPropagationSources
  ConstantPropagationTests.cpp
)

ADD_EXECUTABLE(ConstantPropagationTests ${constantPropagationSources})

TARGET_LINK_LIBRARIES(ConstantPropagationTests naja_dnl naja_opt naja_snl_visual gmock gtest_main)

GTEST_DISCOVER_TESTS(ConstantPropagationTests)