# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_test(NAME snl-python-tests
  COMMAND Python3::Interpreter -m unittest discover -v ${CMAKE_CURRENT_SOURCE_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

set_tests_properties(snl-python-tests
    PROPERTIES ENVIRONMENT
    "PYTHONPATH=${PROJECT_BINARY_DIR}/src/snl/python/snl_wrapping:$ENV{PYTHONPATH};SNL_WRAPPING_TEST_PATH=${CMAKE_CURRENT_BINARY_DIR}"
)