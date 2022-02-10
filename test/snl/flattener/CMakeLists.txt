INCLUDE(GoogleTest)

SET(snl_flattener_tests SNLFlattenerTest0.cpp)

ADD_EXECUTABLE(snlFlattenerTests ${snl_flattener_tests})

TARGET_LINK_LIBRARIES(snlFlattenerTests snl_netlists gtest gtest_main)

GTEST_DISCOVER_TESTS(snlFlattenerTests)