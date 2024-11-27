# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  YosysLibertyParser.cpp  
)

add_library(yosys_liberty STATIC ${SOURCES})
set_property(TARGET yosys_liberty PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(yosys_liberty PUBLIC coverage_config sanitizers_config)
target_include_directories(yosys_liberty
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${FLEX_INCLUDE_DIRS}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT BUILD_NAJA_PYTHON)
  install(TARGETS yosys_liberty LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(NOT BUILD_NAJA_PYTHON)