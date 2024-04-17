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