# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(core_tests
    NajaPrivatePropertyTest.cpp
    NajaPerfTests.cpp
    NajaProxyPropertyTest.cpp
)

ADD_EXECUTABLE(najaCoreTests ${core_tests})

target_compile_definitions(najaCoreTests PRIVATE
    NAJA_CORE_TESTS_PATH="${CMAKE_CURRENT_BINARY_DIR}"
)

TARGET_LINK_LIBRARIES(najaCoreTests naja_snl_python naja_core gmock gtest_main)

GTEST_DISCOVER_TESTS(najaCoreTests)