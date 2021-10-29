INCLUDE(GoogleTest)

SET(snl_vrl_tests SNLVRLDumperTest0.cpp)

ADD_EXECUTABLE(snlVRLTests ${snl_vrl_tests})

TARGET_LINK_LIBRARIES(snlVRLTests naja_snl_verilog gtest gtest_main)

GTEST_DISCOVER_TESTS(snlVRLTests)
