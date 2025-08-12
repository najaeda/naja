# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(sources
  MetricsTests.cpp
)

ADD_EXECUTABLE(najaMetricsTests ${sources})

TARGET_LINK_LIBRARIES(najaMetricsTests naja_dnl naja_opt gmock gtest_main naja_snl_visual naja_metrics)

GTEST_DISCOVER_TESTS(najaMetricsTests)