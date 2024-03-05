# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(snl_visual_tests
  SNLVisualTests.cpp
)

ADD_EXECUTABLE(snl_visual_tests ${snl_visual_tests})

TARGET_LINK_LIBRARIES(snl_visual_tests naja_dnl naja_snl_visual gtest gmock gtest_main)

GTEST_DISCOVER_TESTS(snl_visual_tests)
