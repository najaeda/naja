# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(sources
  BNETests.cpp
)

ADD_EXECUTABLE(BNETests ${sources})

TARGET_LINK_LIBRARIES(BNETests naja_dnl naja_bne gmock gtest_main)

GTEST_DISCOVER_TESTS(BNETests)