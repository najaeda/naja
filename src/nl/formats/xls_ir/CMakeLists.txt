# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  frontend/SNLXLSConstructor.cpp)

set(HEADERS
  frontend/SNLXLSConstructor.h
  frontend/SNLXLSConstructorException.h)

add_library(naja_snl_xls_ir STATIC ${SOURCES})
set_property(TARGET naja_snl_xls_ir PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(naja_snl_xls_ir
  naja_nl_frontend_common
  naja_nl)
target_include_directories(naja_snl_xls_ir PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/frontend)

if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_snl_xls_ir PROPERTIES PUBLIC_HEADER "${HEADERS}")
  install(TARGETS naja_snl_xls_ir
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
