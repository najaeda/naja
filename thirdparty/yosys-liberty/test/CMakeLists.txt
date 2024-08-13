# SPDX-FileCopyrightText: 2024 The Naja liberty authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

include(GoogleTest)

set(tests
  YosysLibertyTest0.cpp
  YosysLibertyTest1.cpp
)

add_executable(yosys_liberty_tests ${tests})
target_compile_definitions(yosys_liberty_tests PRIVATE
  YOSYS_LIBERTY_BENCHMARKS="${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(yosys_liberty_tests yosys_liberty gmock gtest_main)

GTEST_DISCOVER_TESTS(yosys_liberty_tests)