# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.21)

project(naja
  VERSION 0.1.0
  HOMEPAGE_URL https://github.com/najaeda/naja
  LANGUAGES CXX
)

#RPATH settings
#set(MACOSX_RPATH TRUE)
# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)
 
# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
 
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
######

set(CMAKE_CXX_STANDARD 20)

set(NAJA_CXX_WARNINGS -Werror -Wall -pedantic-errors -Wconversion -Wsign-conversion -Wextra -Wno-unused-parameter)

set(ARGPARSE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/argparse-3.0/include)
set(SPDLOG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/spdlog-1.13.0/include)

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

find_package(TBB REQUIRED)
find_package(Boost REQUIRED)
find_package(Python3 3.9...<3.99 COMPONENTS Development Interpreter REQUIRED)
	#PATHS /usr/lib/python3.9.5)
find_package(CapnProto CONFIG REQUIRED)

add_subdirectory(thirdparty)
add_subdirectory(src)
add_subdirectory(cmake)
add_subdirectory(primitives)

if(PROJECT_IS_TOP_LEVEL)
include(CTest)
enable_testing()
add_subdirectory(test)

# Doxygen
# look for Doxygen package
find_package(Doxygen)
if(DOXYGEN_FOUND )
  add_subdirectory(docs)
endif(DOXYGEN_FOUND)
endif(PROJECT_IS_TOP_LEVEL)

