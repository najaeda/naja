# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(HEADERS
  IsoTracer.h
)

# Create interface library for loadless logic analysis
add_library(naja_loadless_analysis INTERFACE)

target_include_directories(naja_loadless_analysis 
  INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Install headers
set_target_properties(naja_loadless_analysis PROPERTIES PUBLIC_HEADER "${HEADERS}")