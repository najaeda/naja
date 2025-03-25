# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(pnl_tests
  PNLPointTest.cpp
  PNLBoxTest.cpp
  PNLDesignTest.cpp
)

ADD_EXECUTABLE(pnlTests ${pnl_tests})
TARGET_LINK_LIBRARIES(pnlTests naja_nl gmock gtest_main)
GTEST_DISCOVER_TESTS(pnlTests)