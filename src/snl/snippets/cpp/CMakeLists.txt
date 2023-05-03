add_executable(snl_snippet ${PROJECT_SOURCE_DIR}/src/snl/snippets/app/src/SNLSnippet.cpp)
target_link_libraries(snl_snippet naja_snl)

add_executable(snl_vrl_snippet ${PROJECT_SOURCE_DIR}/src/snl/snippets/app/src/SNLVRLSnippet.cpp)
target_link_libraries(snl_vrl_snippet naja_snl_verilog naja_snl_primitives argparse)