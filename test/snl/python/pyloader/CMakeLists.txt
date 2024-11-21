# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE(GoogleTest)

SET(snl_pyloader_tests
  SNLPyDBLoaderTest0.cpp
  SNLPyDesignLoaderTest0.cpp
  SNLPrimitivesTest0.cpp
  SNLPrimitivesTest1.cpp
)

ADD_EXECUTABLE(snlPyLoaderTests ${snl_pyloader_tests})
target_compile_definitions(snlPyLoaderTests PRIVATE
  SNL_PRIMITIVES_TEST_PATH="${CMAKE_CURRENT_SOURCE_DIR}")
TARGET_LINK_LIBRARIES(snlPyLoaderTests naja_snl_pyloader gmock gtest_main)

gtest_discover_tests(
  snlPyLoaderTests
  DISCOVERY_TIMEOUT 30
  PROPERTIES ENVIRONMENT
  PYTHONPATH=${PROJECT_BINARY_DIR}/src/snl/python/snl_wrapping
)

SET(snl_pyedit_tests
  SNLPyEditTest0.cpp
)

SET(snl_pyedit_long_tests
  SNLPyHugeMatrixTest.cpp
)
ADD_EXECUTABLE(snlPyEditLongTests ${snl_pyedit_long_tests})
TARGET_LINK_LIBRARIES(snlPyEditLongTests naja_snl_pyloader naja_snl_verilog gmock gtest_main)
target_compile_definitions(snlPyEditLongTests PRIVATE
  SNL_PYEDIT_TEST_PATH="${CMAKE_CURRENT_SOURCE_DIR}"
  SNL_DUMP_PATH="${CMAKE_CURRENT_BINARY_DIR}"
  )
IF(LONG_TESTS)
gtest_discover_tests(
  snlPyEditLongTests
  PROPERTIES ENVIRONMENT
  PYTHONPATH=${PROJECT_BINARY_DIR}/src/snl/python/snl_wrapping
)
ENDIF(LONG_TESTS)

ADD_EXECUTABLE(snlPyEditTests ${snl_pyedit_tests})
target_compile_definitions(snlPyEditTests PRIVATE
  SNL_PYEDIT_TEST_PATH="${CMAKE_CURRENT_SOURCE_DIR}"
  SNL_DUMP_PATH="${CMAKE_CURRENT_BINARY_DIR}"
  )
TARGET_LINK_LIBRARIES(snlPyEditTests naja_snl_pyloader gmock gtest_main)
gtest_discover_tests(
  snlPyEditTests
  PROPERTIES ENVIRONMENT
  PYTHONPATH=${PROJECT_BINARY_DIR}/src/snl/python/snl_wrapping
)
