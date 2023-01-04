set(snl_vrl_constructor_tests
    SNLVRLConstructorTest0.cpp
)

add_executable(snlVRLConstructorTests ${snl_vrl_constructor_tests})

target_compile_definitions(snlVRLConstructorTests PRIVATE
  SNL_VRL_BENCHMARKS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../benchmarks")
target_link_libraries(snlVRLConstructorTests naja_snl_verilog gtest gtest_main)

GTEST_DISCOVER_TESTS(snlVRLConstructorTests)