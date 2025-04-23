# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(naja_if_tests
  NajaIFTest0.cpp
)

add_executable(najaIFTests ${naja_if_tests})
target_compile_definitions(najaIFTests PRIVATE
    NAJA_IF_TEST_PATH="${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(najaIFTests naja_if gmock gtest_main)
GTEST_DISCOVER_TESTS(najaIFTests)