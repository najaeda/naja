# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(NETLIST_GRAPH_SOURCES
  NetlistGraph.cpp
)
set(NETLIST_GRAPH_HEADERS
  NetlistGraph.h NetlistGraph_impl.h
)

add_library(naja_snl_visual STATIC ${NETLIST_GRAPH_SOURCES})
set_property(TARGET naja_snl_visual PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(naja_snl_visual PUBLIC naja_snl)
target_compile_options(naja_snl_visual PRIVATE ${NAJA_CXX_WARNINGS}) 

target_include_directories(naja_snl_visual PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_snl_visual PROPERTIES PUBLIC_HEADER "${NETLIST_GRAPH_HEADERS}")
  install(TARGETS naja_snl_visual
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(NOT BUILD_NAJA_PYTHON)
