# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(snl_visual_tests
  SNLVisualTests.cpp
)

ADD_EXECUTABLE(snl_visual_tests ${snl_visual_tests})

if(ENABLE_TEST_DOT)
  target_compile_definitions(snl_visual_tests PRIVATE
    NAJA_ENABLE_TEST_DOT)
endif()

TARGET_LINK_LIBRARIES(snl_visual_tests naja_dnl naja_snl_visual gmock gtest_main)

GTEST_DISCOVER_TESTS(snl_visual_tests
  DISCOVERY_TIMEOUT 30
)
