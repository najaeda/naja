# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

message(STATUS "CMake version: ${CMAKE_VERSION}")

project(naja
  VERSION 1.0.0
  HOMEPAGE_URL https://github.com/najaeda/naja
)

cmake_policy(SET CMP0048 NEW)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.30")
  cmake_policy(SET CMP0167 OLD)
  cmake_policy(SET CMP0144 NEW)
endif()

include(GNUInstallDirs)

option(BUILD_NAJA_PYTHON "Build Naja Python package" OFF)

#RPATH settings
if(BUILD_NAJA_PYTHON)
  set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
  if(APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path")
  else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
  endif()
  #set(CMAKE_INSTALL_RPATH "$ORIGIN/lib:$ORIGIN")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
else()
  set(CMAKE_MACOSX_RPATH TRUE)
  # use, i.e. don't skip the full RPATH for the build tree
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  # when building, don't use the install RPATH already
  # (but later on when installing)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
  # add the automatically determined parts of the RPATH
  # which point to directories outside the build tree to the install RPATH
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  ######
endif()

######

set(CMAKE_CXX_STANDARD 20)

option(BUILD_ONLY_DOC "Only configure to build Doxygen documentation" OFF)
option(BUILD_BENCHMARKS "Build performance benchmarks" OFF)

set(ARGPARSE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/argparse-3.1/include)
set(SPDLOG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/spdlog-1.17.0/include)
set(LEFDEF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lefdef)

add_library(coverage_config INTERFACE)
option(CODE_COVERAGE "Enable coverage reporting" OFF)
if(CODE_COVERAGE)
  # Add required flags (GCC & LLVM/Clang)
  target_compile_options(coverage_config INTERFACE
    -O0        # no optimization
    -g         # generate debug info
    --coverage # sets all required flags
  )
  target_link_options(coverage_config INTERFACE --coverage)
  target_compile_definitions(coverage_config INTERFACE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
endif(CODE_COVERAGE)

add_library(sanitizers_config INTERFACE)
option(ENABLE_SANITIZERS "Enable sanitizers in compilation" OFF)
if(ENABLE_SANITIZERS)
  target_compile_options(sanitizers_config INTERFACE
    -fsanitize=address
    -fno-omit-frame-pointer
    #-fsanitize=thread
  )
  target_link_options(sanitizers_config INTERFACE
    -fsanitize=address
    #-fsanitize=thread
  )
endif(ENABLE_SANITIZERS)

if(NOT BUILD_ONLY_DOC)
  find_package(Boost REQUIRED)
  find_package(TBB REQUIRED)
  find_package(CapnProto REQUIRED)
  find_package(ZLIB REQUIRED)
  if (NOT ${BUILD_NAJA_PYTHON})
    find_package(Python3 3.9...<3.99 COMPONENTS Interpreter Development.Embed Development.Module REQUIRED)
  else()
    find_package(Python3 3.9...<3.99 COMPONENTS Interpreter Development.Module REQUIRED)
  endif()
endif(NOT BUILD_ONLY_DOC)

if(PROJECT_IS_TOP_LEVEL AND NOT BUILD_NAJA_PYTHON)
  option(LONG_TESTS "Enable long tests" OFF)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()

if(WIN32)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()
  
if(BUILD_ONLY_DOC)
  option(BUILD_SPHINX_DOCUMENTATION "Build Sphinx documentation" OFF)
  
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
  # Doxygen
  # look for Doxygen package
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    add_subdirectory(docs)
  endif(DOXYGEN_FOUND)
  
  if(BUILD_SPHINX_DOCUMENTATION)
    #Sphinx
    #look for Sphinx package
    find_package(Sphinx)
    if (NOT (DOXYGEN_FOUND AND SPHINX_FOUND))
      message(SEND_ERROR "Doxygen and Sphinx must be available to build Sphinx documentation")
    endif(NOT (DOXYGEN_FOUND AND SPHINX_FOUND))
  endif(BUILD_SPHINX_DOCUMENTATION)
endif()

if(NOT BUILD_ONLY_DOC)
  add_subdirectory(thirdparty)
  add_subdirectory(src)
  if(PROJECT_IS_TOP_LEVEL AND BUILD_BENCHMARKS)
    add_subdirectory(bench)
  endif()
  if(NOT BUILD_NAJA_PYTHON)
    add_subdirectory(cmake)
    add_subdirectory(primitives)
  endif(NOT BUILD_NAJA_PYTHON)
endif(NOT BUILD_ONLY_DOC)
