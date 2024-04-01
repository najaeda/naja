# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(PNL_SOURCES
  PNLOrientation.cpp
  PNLDesign.cpp PNLInstance.cpp
)

add_library(naja_pnl SHARED ${PNL_SOURCES})

target_link_libraries(naja_pnl PUBLIC naja_core)
target_compile_options(naja_pnl PRIVATE ${NAJA_CXX_WARNINGS}) 
#target_compile_definitions(naja_snl PRIVATE SNL_DESTROY_DEBUG)