# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES SNLNetlist0.cpp)

add_library(snl_netlists SHARED ${SOURCES})

target_link_libraries(snl_netlists naja_snl)

target_include_directories(snl_netlists PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include(GoogleTest)

set(snl_netlists_tests
  SNLNetlist0Test.cpp
)

add_executable(snlNetlistTests ${snl_netlists_tests})

target_link_libraries(snlNetlistTests snl_netlists gtest gmock gtest_main)

GTEST_DISCOVER_TESTS(snlNetlistTests)