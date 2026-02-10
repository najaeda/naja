# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(snl_sv_constructor_tests
    SNLSVConstructorTestSimple.cpp
    SNLSVConstructorTestElab.cpp
)
add_executable(snlSVConstructorTests ${snl_sv_constructor_tests})
target_compile_definitions(snlSVConstructorTests PRIVATE
  SNL_SV_BENCHMARKS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../benchmarks"
  SNL_SV_DUMPER_TEST_PATH="${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(snlSVConstructorTests naja_snl_systemverilog naja_snl_verilog gmock gtest_main)
gtest_discover_tests(snlSVConstructorTests
  DISCOVERY_TIMEOUT 30
  PROPERTIES ENVIRONMENT
  PYTHONPATH=${PROJECT_BINARY_DIR}/src/nl/python/naja_wrapping)
