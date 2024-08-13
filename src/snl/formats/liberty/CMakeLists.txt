# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  SNLLibertyConstructor.cpp
)

#In the future provide a header exposing only simple parsing interface
#SET(HEADERS
#  frontend/SNLVRLConstructor.h
#  backend/SNLVRLDumper.h
#)

add_library(naja_snl_liberty SHARED ${SOURCES})
target_link_libraries(naja_snl_liberty naja_snl yosys_liberty)

#target_include_directories(naja_snl_verilog PUBLIC
#  ${CMAKE_CURRENT_SOURCE_DIR}/frontend
#  ${CMAKE_CURRENT_SOURCE_DIR}/backend)
#target_include_directories(naja_snl_verilog PUBLIC ${SPDLOG_DIR})

#set_target_properties(naja_snl_verilog PROPERTIES PUBLIC_HEADER "${HEADERS}")
#install(TARGETS naja_snl_verilog LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 