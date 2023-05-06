INCLUDE(GoogleTest)

SET(snl_primitives_tests
  SNLPrimitivesTest0.cpp
)

ADD_EXECUTABLE(snlPrimitivesTests ${snl_primitives_tests})
target_compile_definitions(snlPrimitivesTests PRIVATE
  SNL_PRIMITIVES_TEST_PATH="${CMAKE_CURRENT_SOURCE_DIR}")
TARGET_LINK_LIBRARIES(snlPrimitivesTests naja_snl_primitives gtest gmock gtest_main)

gtest_discover_tests(
  snlPrimitivesTests
  PROPERTIES ENVIRONMENT
  PYTHONPATH=${PROJECT_BINARY_DIR}/src/snl/python/snl_wrapping
)