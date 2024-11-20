# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

if(BUILD_NAJA_PYTHON)
  add_subdirectory(naja-verilog EXCLUDE_FROM_ALL)
else(BUILD_NAJA_PYTHON)
  add_subdirectory(naja-verilog)
endif(BUILD_NAJA_PYTHON)
add_subdirectory(yosys-liberty)
add_subdirectory(googletest EXCLUDE_FROM_ALL)
#add_subdirectory(cpptrace EXCLUDE_FROM_ALL)
