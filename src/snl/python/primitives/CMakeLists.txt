add_library(naja_snl_primitives SHARED SNLPrimitivesLoader.cpp)

target_link_libraries(naja_snl_primitives PUBLIC naja_snl PRIVATE naja_snl_python Python3::Python)

target_include_directories(naja_snl_primitives PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(naja_snl_primitives PROPERTIES PUBLIC_HEADER SNLPrimitivesLoader.h)
install(TARGETS naja_snl_primitives LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 