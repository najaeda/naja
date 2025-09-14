# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Add subdirectories
add_subdirectory(core)
add_subdirectory(truthtable)

# Create library for reduction optimization module
add_library(naja_reduction INTERFACE)

# Link with sub-components and external dependencies
target_link_libraries(naja_reduction 
  INTERFACE 
    naja_reduction_core
    naja_reduction_truthtable
)

# Include directories
target_include_directories(naja_reduction 
  INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

