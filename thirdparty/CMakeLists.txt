# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(yosys-liberty)
add_subdirectory(naja-verilog)
add_subdirectory(googletest EXCLUDE_FROM_ALL)
if (NOT BUILD_EMSCRIPTEN AND NOT BUILD_NAJA_PYTHON)
  add_subdirectory(lefdef EXCLUDE_FROM_ALL)
endif()
#set(CPPTRACE_GET_SYMBOLS_WITH_LIBDWARF OFF CACHE BOOL "" FORCE)
#add_subdirectory(cpptrace EXCLUDE_FROM_ALL)