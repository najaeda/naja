# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Add subdirectories
add_subdirectory(analysis)
add_subdirectory(computation)

# Sources for this module
set(SOURCES
  ConstantPropagation.cpp
)

set(HEADERS
  ConstantPropagation.h
)

# Create library for constant propagation module
add_library(naja_constant_propagation SHARED ${SOURCES})

# Link with sub-components and external dependencies
target_link_libraries(naja_constant_propagation 
  PUBLIC 
    naja_constant_analysis
    naja_constant_computation
    naja_reduction_core  # For Reduction.h dependency
    naja_bne 
    naja_dnl
)

# Include directories
target_include_directories(naja_constant_propagation 
  PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install headers
set_target_properties(naja_constant_propagation PROPERTIES PUBLIC_HEADER "${HEADERS}")

if(APPLE)
  target_compile_options(naja_constant_propagation PRIVATE -Wno-sign-conversion)
endif()