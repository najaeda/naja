# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Set the list of Python source files
file(GLOB_RECURSE PYTHON_FILES
    "${CMAKE_SOURCE_DIR}/src/najaeda/najaeda/*.py"
)

# Command to create the simulated naja python package structure
add_custom_target(najaeda-python-tests-structure
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/najaeda/najaeda ${CMAKE_CURRENT_BINARY_DIR}/najaeda
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/src/nl/python/naja_wrapping/naja.so ${CMAKE_CURRENT_BINARY_DIR}/najaeda/naja.so
  COMMAND Python3::Interpreter -m unittest discover -v ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${PYTHON_FILES} 
)

add_test(NAME najaeda-python-tests
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target najaeda-python-tests-structure
  #WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
# Ensure structure is created before tests
#add_dependencies(naja-python-tests simulate_package_structure)

set_tests_properties(najaeda-python-tests PROPERTIES
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR};VERILOG_BENCHMARKS_PATH=${CMAKE_SOURCE_DIR}/test/nl/formats/verilog/benchmarks;SYSTEMVERILOG_BENCHMARKS_PATH=${CMAKE_SOURCE_DIR}/test/nl/formats/systemverilog/benchmarks;LIBERTY_BENCHMARKS_PATH=${CMAKE_SOURCE_DIR}/test/nl/formats/liberty/benchmarks;NAJAEDA_TEST_PATH=${CMAKE_CURRENT_BINARY_DIR};NAJAEDA_SOURCE_TEST_PATH=${CMAKE_CURRENT_SOURCE_DIR}"
)
