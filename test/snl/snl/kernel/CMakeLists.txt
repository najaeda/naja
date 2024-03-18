# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(snl_tests
  SNLIDTest.cpp
  SNLUniverseTest.cpp SNLDBTest.cpp SNLLibraryTest.cpp
  SNLDesignTest.cpp SNLDesignUniquificationTest.cpp
  SNLTermTest.cpp
  SNLInstanceTest0.cpp SNLInstanceTest1.cpp SNLInstanceTest2.cpp
  SNLNetTest.cpp
  SNLPathTest0.cpp SNLPathTest1.cpp SNLPathTest2.cpp
  SNLOccurrenceTest.cpp
  SNLParameterTest.cpp
  SNLDB0Test.cpp
  SNLMergeAssignsTest.cpp
)

SET(snl_tests_with_pyloader
  SNLEquipotentialTest.cpp
)

ADD_EXECUTABLE(snlTests ${snl_tests})
target_compile_definitions(snlTests PRIVATE
  SNL_BENCHS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/benchs")

ADD_EXECUTABLE(snlTestsWithPyLoader ${snl_tests_with_pyloader})
target_compile_definitions(snlTestsWithPyLoader PRIVATE
  SNL_BENCHS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/benchs")

TARGET_LINK_LIBRARIES(snlTests naja_snl gmock gtest_main)
TARGET_LINK_LIBRARIES(snlTestsWithPyLoader naja_snl_pyloader gmock gtest_main)

GTEST_DISCOVER_TESTS(snlTests)
GTEST_DISCOVER_TESTS(snlTestsWithPyLoader
  PROPERTIES ENVIRONMENT
  PYTHONPATH=${PROJECT_BINARY_DIR}/src/snl/python/snl_wrapping
)