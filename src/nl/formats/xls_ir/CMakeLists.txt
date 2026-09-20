# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(CAPNPC_SRC_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/frontend")
set(CAPNPC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
capnp_generate_cpp(xlsIRBridgeSources xlsIRBridgeHeaders
  frontend/xls_ir_bridge.capnp)

add_library(naja_xls_ir_bridge_capnp STATIC
  ${xlsIRBridgeSources} ${xlsIRBridgeHeaders})
set_property(TARGET naja_xls_ir_bridge_capnp
  PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(naja_xls_ir_bridge_capnp PUBLIC CapnProto::capnp)
target_include_directories(naja_xls_ir_bridge_capnp PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR})

set(SOURCES
  frontend/SNLXLSConstructor.cpp
  frontend/SNLXLSIRReader.cpp)

set(HEADERS
  frontend/SNLXLSConstructor.h
  frontend/SNLXLSConstructorException.h
  frontend/SNLXLSIRReader.h)

add_library(naja_snl_xls_ir STATIC ${SOURCES})
set_property(TARGET naja_snl_xls_ir PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(naja_snl_xls_ir
  PUBLIC
    naja_nl_frontend_common
    naja_nl
  PRIVATE naja_xls_ir_bridge_capnp)
target_include_directories(naja_snl_xls_ir PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/frontend)

if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_snl_xls_ir PROPERTIES PUBLIC_HEADER "${HEADERS}")
  install(TARGETS naja_snl_xls_ir
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
