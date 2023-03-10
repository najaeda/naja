add_executable(snl_app SNLSnippet.cpp SNLUniverseSnippet)
target_include_directories(snl_app SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(snl_app Naja::SNL)

add_executable(snl_vrl_app SNLVRLSnippet.cpp)
target_include_directories(snl_vrl_app SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(snl_vrl_app Naja::SNLVerilog)