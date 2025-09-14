# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Placeholder for future removal implementation files

# Create interface library for removal implementation
add_library(naja_loadless_removal INTERFACE)

target_include_directories(naja_loadless_removal 
  INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)