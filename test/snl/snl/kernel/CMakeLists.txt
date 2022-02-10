INCLUDE(GoogleTest)

SET(snl_tests
    SNLUniverseTest.cpp SNLDBTest.cpp SNLLibraryTest.cpp
    SNLDesignTest.cpp SNLTermTest.cpp
    SNLInstanceTest.cpp
    SNLNetTest.cpp #SNLLargeDBTest.cpp
    #SNLPathTest.cpp
    SNLParameterTest.cpp
)

ADD_EXECUTABLE(snlTests ${snl_tests})

TARGET_LINK_LIBRARIES(snlTests naja_snl gtest gmock gtest_main)

GTEST_DISCOVER_TESTS(snlTests)