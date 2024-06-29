# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(pnl_tests
  PNLDesignTest.cpp
)

ADD_EXECUTABLE(pnlTests ${pnl_tests})

TARGET_LINK_LIBRARIES(pnlTests naja_snl gmock gtest_main)

GTEST_DISCOVER_TESTS(pnlTests)