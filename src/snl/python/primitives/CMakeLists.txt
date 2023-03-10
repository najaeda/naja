add_library(naja_snl_primitives SHARED SNLPrimitivesLoader.cpp)

target_link_libraries(naja_snl_primitives PUBLIC naja_snl PRIVATE naja_snl_python Python3::Python)

target_include_directories(naja_snl_primitives PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})