# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.12)
project(naja_snl_liberty LANGUAGES CXX)

# Sources and headers
set(SOURCES
  SNLLibertyConstructor.cpp
  SNLBooleanTree.cpp
)

set(HEADERS
  SNLLibertyConstructor.h
)

# Find Boost iostreams and zlib (non-fatal; we will link if found)
find_package(Boost COMPONENTS iostreams REQUIRED)
find_package(ZLIB REQUIRED)

# Create static library
add_library(naja_snl_liberty STATIC ${SOURCES})
set_property(TARGET naja_snl_liberty PROPERTY POSITION_INDEPENDENT_CODE ON)

# Public include directory for consumers/tests
target_include_directories(naja_snl_liberty PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Always link internal project deps (these should be existing targets in the superproject)
# Keep yosys_liberty PUBLIC so its include interface is available to consumers/tests.
target_link_libraries(naja_snl_liberty
  PUBLIC
  naja_nl
  yosys_liberty
)

# Conditionally link Boost.Iostreams (prefer imported target if available)
if(TARGET Boost::iostreams)
  target_link_libraries(naja_snl_liberty PRIVATE Boost::iostreams)
elseif(DEFINED Boost_IOSTREAMS_LIBRARY AND Boost_IOSTREAMS_LIBRARY)
  target_link_libraries(naja_snl_liberty PRIVATE "${Boost_IOSTREAMS_LIBRARY}")
else()
  message(STATUS "Boost.Iostreams not found; gzip/zlib decompression support will be unavailable.")
endif()

# Conditionally link zlib (required by Boost iostreams at runtime)
if(TARGET ZLIB::ZLIB)
  target_link_libraries(naja_snl_liberty PRIVATE ZLIB::ZLIB)
elseif(DEFINED ZLIB_LIBRARIES AND ZLIB_LIBRARIES)
  target_link_libraries(naja_snl_liberty PRIVATE "${ZLIB_LIBRARIES}")
else()
  message(STATUS "zlib not found; install zlib development package if you need compression support.")
endif()

# Compile settings
target_compile_features(naja_snl_liberty PUBLIC cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
  target_compile_options(naja_snl_liberty PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# Install static archive and public headers when not building the python variant
if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_snl_liberty PROPERTIES PUBLIC_HEADER "${HEADERS}")
  install(TARGETS naja_snl_liberty
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

# Optional alias for consumers
add_library(naja::snl_liberty ALIAS naja_snl_liberty)
