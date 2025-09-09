# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(yosys-liberty)
if (NOT EMSCRIPTEN_BUILD)
  add_subdirectory(naja-verilog)
  add_subdirectory(lefdef EXCLUDE_FROM_ALL)
  add_subdirectory(googletest EXCLUDE_FROM_ALL)
endif()
#add_subdirectory(cpptrace EXCLUDE_FROM_ALL)
