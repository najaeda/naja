# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(HEADERS
  OutputValueComputer.h
)

# Create interface library for computation
add_library(naja_constant_computation INTERFACE)

target_include_directories(naja_constant_computation 
  INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install headers
set_target_properties(naja_constant_computation PROPERTIES PUBLIC_HEADER "${HEADERS}")