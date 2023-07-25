INCLUDE(GoogleTest)

SET(snl_tests
  SNLIDTest.cpp
  SNLUniverseTest.cpp SNLDBTest.cpp SNLLibraryTest.cpp
  SNLDesignTest.cpp SNLTermTest.cpp
  SNLInstanceTest0.cpp SNLInstanceTest1.cpp SNLInstanceTest2.cpp
  SNLNetTest.cpp #SNLLargeDBTest.cpp
  SNLPathTest0.cpp SNLPathTest1.cpp
  SNLOccurrenceTest.cpp
  SNLEquipotentialTest.cpp
  SNLParameterTest.cpp
  SNLDB0Test.cpp
)

ADD_EXECUTABLE(snlTests ${snl_tests})
target_compile_definitions(snlTests PRIVATE
  SNL_BENCHS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/benchs")

TARGET_LINK_LIBRARIES(snlTests naja_snl_pyloader gtest gmock gtest_main)

GTEST_DISCOVER_TESTS(snlTests
  PROPERTIES ENVIRONMENT
  PYTHONPATH=${PROJECT_BINARY_DIR}/src/snl/python/snl_wrapping
)