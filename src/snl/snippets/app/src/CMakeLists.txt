add_executable(snl_app SNLSnippet.cpp)
target_include_directories(snl_app SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(snl_app Naja::Naja)

add_executable(snl_vrl_app SNLVRLSnippet.cpp)
target_include_directories(snl_vrl_app SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(snl_vrl_app Naja::NajaVerilog Naja::Naja)