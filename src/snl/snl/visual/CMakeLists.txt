# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(NETLIST_GTAPH_SOURCES
  NetlistGraph.cpp
)
set(NETLIST_GTAPH_HEADERS
  NetlistGraph.h NetlistGraph_impl.h
)

add_library(naja_visual SHARED ${NETLIST_GTAPH_SOURCES})

target_link_libraries(naja_visual PUBLIC naja_core naja_snl tbb tbbmalloc)
target_compile_options(naja_visual PRIVATE ${NAJA_CXX_WARNINGS}) 
#target_compile_definitions(naja_visual PRIVATE SNL_DESTROY_DEBUG)

target_include_directories(naja_visual SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_visual SYSTEM BEFORE PUBLIC ${TBB_INCLUDE_DIR})
target_include_directories(naja_visual PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(naja_visual PROPERTIES PUBLIC_HEADER "${NETLIST_GTAPH_HEADERS}")
install(TARGETS naja_visual LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 