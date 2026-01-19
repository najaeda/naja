# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(GRAPH_SOURCES
  Edge.cpp
  Graph.cpp
  Node.cpp
)

add_library(naja_tgraph SHARED ${GRAPH_SOURCES})

#target_link_libraries(naja_tgraph PUBLIC naja_nl TBB::tbb TBB::tbbmalloc)

target_compile_options(naja_tgraph BEFORE PRIVATE ${NAJA_CXX_WARNINGS}) 

if(BUILD_NAJA_PYTHON)
  install(TARGETS naja_tgraph
    LIBRARY DESTINATION najaeda
    RUNTIME DESTINATION najaeda
  )
else(BUILD_NAJA_PYTHON)
  set_target_properties(naja_tgraph PROPERTIES PUBLIC_HEADER "${GRAPH_HEADERS}")
  install(TARGETS naja_tgraph
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(BUILD_NAJA_PYTHON)
