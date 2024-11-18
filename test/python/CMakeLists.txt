add_test(NAME naja-python-tests
  COMMAND Python3::Interpreter -m unittest discover -v ${CMAKE_CURRENT_SOURCE_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

set_tests_properties(naja-python-tests
    PROPERTIES ENVIRONMENT
    "PYTHONPATH=${PROJECT_BINARY_DIR}/src/naja.python/$ENV{PYTHONPATH};SNL_WRAPPING_TEST_PATH=${CMAKE_CURRENT_BINARY_DIR}"
)
