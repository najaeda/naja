# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
cmake_policy(SET CMP0048 NEW)

project(naja
  VERSION 1.0.0
  HOMEPAGE_URL https://github.com/najaeda/naja
)

message(STATUS "CMake version: ${CMAKE_VERSION}")

set(ARGPARSE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/argparse-3.1/include)
set(SPDLOG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/spdlog-1.15.3/include)
set(LEFDEF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lefdef)

include(GNUInstallDirs)
include("${CMAKE_SOURCE_DIR}/build_config.cmake")

add_library(coverage_config INTERFACE)
add_library(sanitizers_config INTERFACE)
