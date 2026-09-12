# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

if(NOT NAJA_PYTHON_EXECUTABLE)
  message(FATAL_ERROR "NAJA_PYTHON_EXECUTABLE is not set")
endif()

add_test(NAME naja-python-tests
  COMMAND "${NAJA_PYTHON_EXECUTABLE}" -m unittest discover -v ${CMAKE_CURRENT_SOURCE_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

set_tests_properties(naja-python-tests
    PROPERTIES ENVIRONMENT
    "PYTHONPATH=${PROJECT_BINARY_DIR}/src/nl/python/naja_wrapping:$ENV{PYTHONPATH};SNL_WRAPPING_TEST_PATH=${CMAKE_CURRENT_BINARY_DIR};FORMATS_PATH=${CMAKE_CURRENT_SOURCE_DIR}/../../formats"
)

add_executable(najaNativeAPIC NajaNativeAPICTest.c)
target_link_libraries(najaNativeAPIC PRIVATE naja_python_native_api)
add_test(NAME najaNativeAPIC COMMAND najaNativeAPIC)

add_executable(najaNativeAPICXX NajaNativeAPICXXTest.cpp)
target_link_libraries(najaNativeAPICXX PRIVATE naja_python_native_api)
add_test(NAME najaNativeAPICXX COMMAND najaNativeAPICXX)

Python3_add_library(
  naja_native_api_consumer MODULE NajaNativeAPIConsumer.cpp)
target_link_libraries(
  naja_native_api_consumer PRIVATE naja_python_native_api naja_nl)

SET(naja_python_property_tests
  NajaPythonPropertyTest.cpp
)

ADD_EXECUTABLE(najaPythonPropertyTests ${naja_python_property_tests})

target_compile_definitions(najaCoreTests PRIVATE
    PROXY_PROPERTY_TESTS_PATH="${CMAKE_CURRENT_BINARY_DIR}"
)

TARGET_LINK_LIBRARIES(najaPythonPropertyTests naja_python gmock gtest_main)

GTEST_DISCOVER_TESTS(najaPythonPropertyTests)
