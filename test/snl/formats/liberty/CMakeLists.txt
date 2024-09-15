# SPDX-FileCopyrightText: 2024 The Naja liberty authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

include(GoogleTest)

set(tests
  SNLLibertyConstructorTest0.cpp
)

add_executable(snl_liberty_tests ${tests})
target_compile_definitions(snl_liberty_tests PRIVATE
  SNL_LIBERTY_BENCHMARKS="${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(snl_liberty_tests naja_snl_liberty gmock gtest_main)

GTEST_DISCOVER_TESTS(snl_liberty_tests)