set(SOURCES
  SNLFlattener.cpp
  SNLFlattenerInstanceTreeNode.cpp 
  SNLFlattenerInstanceTree.cpp
  SNLFlattenerNetTree.cpp
  SNLFlattenerNetTreeNode.cpp
)

add_library(naja_snl_flattener SHARED ${SOURCES})
target_link_libraries(naja_snl_flattener naja_snl)

target_compile_options(naja_snl_flattener PRIVATE ${NAJA_CXX_WARNINGS})