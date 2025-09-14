# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Add subdirectories
add_subdirectory(analysis)
add_subdirectory(removal)

# Sources for this module
set(SOURCES
  RemoveLoadlessLogic.cpp
)

set(HEADERS
  RemoveLoadlessLogic.h
)

# Create library for loadless logic removal module
add_library(naja_loadless_logic SHARED ${SOURCES})

# Link with sub-components and external dependencies
target_link_libraries(naja_loadless_logic 
  PUBLIC 
    naja_loadless_analysis
    naja_loadless_removal
    naja_bne 
    naja_dnl
)

# Include directories
target_include_directories(naja_loadless_logic 
  PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install headers
set_target_properties(naja_loadless_logic PROPERTIES PUBLIC_HEADER "${HEADERS}")

if(APPLE)
  target_compile_options(naja_loadless_logic PRIVATE -Wno-sign-conversion)
endif()