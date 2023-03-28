set(snl_vrl_constructor_tests
    SNLVRLConstructorUtilsTest0.cpp
    SNLVRLConstructorTest0.cpp
    SNLVRLConstructorTest1.cpp
    SNLVRLConstructorTestErrors.cpp
)

add_executable(snlVRLConstructorTests ${snl_vrl_constructor_tests})

target_compile_definitions(snlVRLConstructorTests PRIVATE
  SNL_VRL_BENCHMARKS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../benchmarks")
target_link_libraries(snlVRLConstructorTests naja_snl_verilog naja_snl_primitives gtest gtest_main)

GTEST_DISCOVER_TESTS(snlVRLConstructorTests
  PROPERTIES ENVIRONMENT
  PYTHONPATH=${PROJECT_BINARY_DIR}/src/snl/python/snl_wrapping)