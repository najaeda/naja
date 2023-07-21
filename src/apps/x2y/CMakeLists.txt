add_executable(naja_x2y NajaX2Y.cpp)
target_include_directories(naja_x2y SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(naja_x2y naja_snl_verilog naja_snl_primitives naja_snl_dump argparse)