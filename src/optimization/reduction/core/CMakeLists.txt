# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  Reduction.cpp
)

set(HEADERS
  Reduction.h
)

# Create library for reduction core
add_library(naja_reduction_core SHARED ${SOURCES})

target_link_libraries(naja_reduction_core 
  PUBLIC 
    naja_bne
)

target_include_directories(naja_reduction_core 
  PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install headers
set_target_properties(naja_reduction_core PROPERTIES PUBLIC_HEADER "${HEADERS}")

if(APPLE)
  target_compile_options(naja_reduction_core PRIVATE -Wno-sign-conversion)
endif()