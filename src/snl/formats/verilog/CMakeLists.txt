# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  frontend/SNLVRLConstructor.cpp
  frontend/SNLVRLConstructorUtils.cpp
  backend/SNLVRLDumper.cpp
)

#In the future provide a header exposing only simple parsing interface
SET(HEADERS
  frontend/SNLVRLConstructor.h
  backend/SNLVRLDumper.h
)

add_library(naja_snl_verilog STATIC ${SOURCES})
target_link_libraries(naja_snl_verilog naja_verilog naja_snl)
set_property(TARGET naja_snl_verilog PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(naja_snl_verilog PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/frontend
  ${CMAKE_CURRENT_SOURCE_DIR}/backend)
target_include_directories(naja_snl_verilog PUBLIC ${SPDLOG_DIR})

if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_snl_verilog PROPERTIES PUBLIC_HEADER "${HEADERS}")
endif(NOT BUILD_NAJA_PYTHON)

install(TARGETS naja_snl_verilog
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
