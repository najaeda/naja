# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(snl_decorators_tests
  SNLDesignModelingTest0.cpp
  SNLTruthTableTest.cpp
  SNLDesignTruthTablesTest0.cpp
  SNLDesignTruthTablesTest1.cpp
  SNLLibraryTruthTablesTest.cpp
  SNLGateTruthTablesTest.cpp
)

ADD_EXECUTABLE(snlDecoratorsTests ${snl_decorators_tests})

TARGET_LINK_LIBRARIES(snlDecoratorsTests naja_nl gmock gtest_main)

GTEST_DISCOVER_TESTS(snlDecoratorsTests)