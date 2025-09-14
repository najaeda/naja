# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(HEADERS
  ConstantCollector.h
)

# Create interface library for constant analysis
add_library(naja_constant_analysis INTERFACE)

target_include_directories(naja_constant_analysis 
  INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install headers
set_target_properties(naja_constant_analysis PROPERTIES PUBLIC_HEADER "${HEADERS}")