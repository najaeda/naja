# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  TruthTableReducer.cpp
)

set(HEADERS
  TruthTableReducer.h
)

# Create library for truth table reduction
add_library(naja_reduction_truthtable SHARED ${SOURCES})

target_link_libraries(naja_reduction_truthtable 
  PUBLIC 
    naja_nl
)

target_include_directories(naja_reduction_truthtable 
  PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install headers
set_target_properties(naja_reduction_truthtable PROPERTIES PUBLIC_HEADER "${HEADERS}")

if(APPLE)
  target_compile_options(naja_reduction_truthtable PRIVATE -Wno-sign-conversion)
endif()