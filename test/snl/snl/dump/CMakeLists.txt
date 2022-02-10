INCLUDE(GoogleTest)

SET(snl_dump_tests
  SNLDumpTest0.cpp
)

ADD_EXECUTABLE(snlDumpTests ${snl_dump_tests})

TARGET_LINK_LIBRARIES(snlDumpTests naja_snl gtest gmock gtest_main)

GTEST_DISCOVER_TESTS(snlDumpTests)