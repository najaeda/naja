# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  LEFConstructor.cpp
  LEFDumper.cpp
  DEFConstructor.cpp
  DEFDumper.cpp
)

#In the future provide a header exposing only simple parsing interface
SET(HEADERS
  LEFConstructor.h
  LEFDumper.h
  DEFConstructor.h
  DEFDumper.h
)

add_library(naja_lefdef STATIC ${SOURCES})

set_property(TARGET naja_lefdef PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(naja_lefdef naja_verilog naja_nl lef def)

target_include_directories(naja_lefdef PUBLIC  ${Boost_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${SPDLOG_DIR} ${LEFDEF_DIR}/src/def/def ${LEFDEF_DIR}/src/lef/lef ${LEFDEF_DIR}/src/)

if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_lefdef PROPERTIES PUBLIC_HEADER "${HEADERS}")
  install(TARGETS naja_lefdef
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(NOT BUILD_NAJA_PYTHON)