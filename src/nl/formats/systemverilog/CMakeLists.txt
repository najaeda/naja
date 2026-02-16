# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  frontend/SNLSVConstructor.cpp
)

set(HEADERS
  frontend/SNLSVConstructor.h
  frontend/SNLSVConstructorException.h
)

add_library(naja_snl_systemverilog STATIC ${SOURCES})
set_property(TARGET naja_snl_systemverilog PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(naja_snl_systemverilog naja_nl slang::slang)

target_include_directories(naja_snl_systemverilog PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/frontend)
target_include_directories(naja_snl_systemverilog PUBLIC ${SPDLOG_DIR})

if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_snl_systemverilog PROPERTIES PUBLIC_HEADER "${HEADERS}")
  install(TARGETS naja_snl_systemverilog
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(NOT BUILD_NAJA_PYTHON)
