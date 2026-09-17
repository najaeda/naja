# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_executable(snlXLSConstructorTests SNLXLSConstructorTest.cpp)
target_link_libraries(snlXLSConstructorTests
  naja_snl_xls_ir
  gtest_main)
gtest_discover_tests(snlXLSConstructorTests)
