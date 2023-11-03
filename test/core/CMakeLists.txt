# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(core_tests
    NajaPrivatePropertyTest.cpp
)

ADD_EXECUTABLE(najaCoreTests ${core_tests})

TARGET_LINK_LIBRARIES(najaCoreTests naja_core gtest gmock gtest_main)

GTEST_DISCOVER_TESTS(najaCoreTests)