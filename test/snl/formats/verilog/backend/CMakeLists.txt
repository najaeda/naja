include(GoogleTest)

set(snl_vrl_tests SNLVRLDumperTest0.cpp)

add_executable(snlVRLTests ${snl_vrl_tests})

target_link_libraries(snlVRLTests naja_snl_verilog gtest gtest_main)

GTEST_DISCOVER_TESTS(snlVRLTests)