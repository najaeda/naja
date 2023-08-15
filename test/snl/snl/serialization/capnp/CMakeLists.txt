include(GoogleTest)

set(snl_capnp_tests
    SNLDumpManifestTest.cpp
    SNLCapnPTest0.cpp
    SNLCapnPTest1.cpp
)

add_executable(snlCapnPTests ${snl_capnp_tests})

target_compile_definitions(snlCapnPTests PRIVATE
    SNL_CAPNP_TEST_PATH="${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(snlCapnPTests naja_snl_dump gtest gmock gtest_main)

GTEST_DISCOVER_TESTS(snlCapnPTests)