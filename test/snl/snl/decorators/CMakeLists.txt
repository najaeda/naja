INCLUDE(GoogleTest)

SET(snl_decorators_tests
  SNLDesignModelingTest0.cpp
)

ADD_EXECUTABLE(snlDecoratorsTests ${snl_decorators_tests})

TARGET_LINK_LIBRARIES(snlDecoratorsTests naja_snl gtest gmock gtest_main)

GTEST_DISCOVER_TESTS(snlDecoratorsTests)