# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_executable(snlDesignBuilderTests SNLDesignBuilderTest.cpp)
target_link_libraries(snlDesignBuilderTests
  naja_nl_frontend_common
  gtest_main)
gtest_discover_tests(snlDesignBuilderTests)
