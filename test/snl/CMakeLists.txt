INCLUDE(GoogleTest)

SET(snl_tests SNLLibraryTest.cpp SNLDesignTest.cpp SNLInstanceTest.cpp SNLNetTest.cpp #SNLLargeDBTest.cpp
)

ADD_EXECUTABLE(snlTests ${snl_tests})

TARGET_LINK_LIBRARIES(snlTests naja_snl gtest gtest_main)

GTEST_DISCOVER_TESTS(snlTests)
