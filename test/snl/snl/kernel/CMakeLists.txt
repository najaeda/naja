INCLUDE(GoogleTest)

SET(snl_tests
    SNLIDTest.cpp
    SNLUniverseTest.cpp SNLDBTest.cpp SNLLibraryTest.cpp
    SNLDesignTest.cpp SNLTermTest.cpp
    SNLInstanceTest0.cpp SNLInstanceTest1.cpp
    SNLNetTest.cpp #SNLLargeDBTest.cpp
    #SNLPathTest.cpp
    SNLParameterTest.cpp
    SNLDB0Test.cpp
)

ADD_EXECUTABLE(snlTests ${snl_tests})

TARGET_LINK_LIBRARIES(snlTests naja_snl gtest gmock gtest_main)

GTEST_DISCOVER_TESTS(snlTests)