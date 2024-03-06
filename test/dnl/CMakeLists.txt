# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(dnl_tests
  DNLTests.cpp
)

ADD_EXECUTABLE(dnlTests ${dnl_tests})

TARGET_LINK_LIBRARIES(dnlTests naja_dnl gtest gmock gtest_main)

GTEST_DISCOVER_TESTS(dnlTests)