# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(yosys-liberty)
add_subdirectory(naja-verilog)
# libnaja_python is dlopened by Python; on musl (Alpine), slang+mimalloc can
# trigger "initial-exec TLS resolves to dynamic definition" relocation errors.
set(SLANG_USE_MIMALLOC OFF CACHE BOOL "Disable mimalloc in embedded slang" FORCE)
set(SLANG_PIC_ON ON CACHE BOOL "Enable position independent code for slang" FORCE)
add_subdirectory(slang)
if(TARGET boost_unordered)
  # Slang's Boost fallback combines a vendored boost_unordered.hpp with a
  # standalone Boost.Regex checkout. If an older system Boost is also present,
  # Boost.Regex can otherwise include system Boost.Exception and collide with
  # the Boost.Exception copy embedded in boost_unordered.hpp.
  target_compile_definitions(boost_unordered INTERFACE BOOST_REGEX_STANDALONE)
  # Slang's FlatMap/ConcurrentMap headers use __has_include to decide whether
  # to include boost/unordered headers. Old system Boost installs can satisfy
  # that check but still miss APIs used by current slang, so make those include
  # paths resolve to slang's vendored Boost.Unordered snapshot in fallback mode.
  target_include_directories(
    boost_unordered BEFORE
    INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/slang-boost-shims/include>")
endif()
add_subdirectory(googletest EXCLUDE_FROM_ALL)
if (NOT BUILD_EMSCRIPTEN AND NOT BUILD_NAJA_PYTHON)
  add_subdirectory(lefdef EXCLUDE_FROM_ALL)
endif()
#set(CPPTRACE_GET_SYMBOLS_WITH_LIBDWARF OFF CACHE BOOL "" FORCE)
#add_subdirectory(cpptrace EXCLUDE_FROM_ALL)
