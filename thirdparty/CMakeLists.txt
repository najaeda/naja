# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(yosys-liberty)
add_subdirectory(naja-verilog)
set(SLANG_INCLUDE_TOOLS OFF CACHE BOOL "" FORCE)
set(SLANG_INCLUDE_TESTS OFF CACHE BOOL "" FORCE)
set(SLANG_INCLUDE_DOCS OFF CACHE BOOL "" FORCE)
set(SLANG_INCLUDE_PYLIB OFF CACHE BOOL "" FORCE)
set(SLANG_INCLUDE_PYTHON_DOCS OFF CACHE BOOL "" FORCE)
set(SLANG_INCLUDE_INSTALL OFF CACHE BOOL "" FORCE)
set(SLANG_USE_MIMALLOC OFF CACHE BOOL "" FORCE)
set(SLANG_USE_CPPTRACE OFF CACHE BOOL "" FORCE)
add_subdirectory(slang EXCLUDE_FROM_ALL)
add_subdirectory(googletest EXCLUDE_FROM_ALL)
if (NOT BUILD_EMSCRIPTEN AND NOT BUILD_NAJA_PYTHON)
  add_subdirectory(lefdef EXCLUDE_FROM_ALL)
endif()
#set(CPPTRACE_GET_SYMBOLS_WITH_LIBDWARF OFF CACHE BOOL "" FORCE)
#add_subdirectory(cpptrace EXCLUDE_FROM_ALL)
