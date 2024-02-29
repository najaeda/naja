# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(dnl_tests
  DNLTests.cpp
)

ADD_EXECUTABLE(dnlTests ${dnl_tests})

target_include_directories(naja_dnl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
TARGET_LINK_LIBRARIES(dnlTests naja_snl naja_dnl gtest gmock gtest_main tbb)

GTEST_DISCOVER_TESTS(dnlTests)
