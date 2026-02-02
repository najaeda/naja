# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  SNLLibertyConstructor.cpp SNLBooleanTree.cpp
)

SET(HEADERS SNLLibertyConstructor.h)

add_library(naja_snl_liberty STATIC ${SOURCES})
set_property(TARGET naja_snl_liberty PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(naja_snl_liberty
  PUBLIC
  naja_nl
  yosys_liberty
  PRIVATE
  ZLIB::ZLIB
)

target_compile_definitions(naja_snl_liberty PRIVATE NAJA_HAVE_ZLIB)

target_include_directories(naja_snl_liberty PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_snl_liberty PROPERTIES PUBLIC_HEADER "${HEADERS}")
  install(TARGETS naja_snl_liberty
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(NOT BUILD_NAJA_PYTHON)
