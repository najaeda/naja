# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

message(STATUS "CMake version: ${CMAKE_VERSION}")

project(naja-if)

set(CMAKE_CXX_STANDARD 20)
find_package(CapnProto REQUIRED)

set(CAPNPC_SRC_PREFIX "${PROJECT_SOURCE_DIR}/../schema" CACHE STRING "" FORCE)
set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

capnp_generate_cpp(NajaSNLCommonSources NajaSNKCommonHeaders ${CAPNPC_SRC_PREFIX}/snl_common.capnp)
capnp_generate_cpp(NajaSNLInterfaceSources NajaSNLInterfaceHeaders ${CAPNPC_SRC_PREFIX}/snl_interface.capnp)
capnp_generate_cpp(NajaSNLImplementationSources NajaSNLImplementationHeaders ${CAPNPC_SRC_PREFIX}/snl_implementation.capnp)

add_library(naja_if STATIC
    ${NajaSNLCommonSources} ${NajaSNLInterfaceSources} ${NajaSNLImplementationSources}
)

target_link_libraries(naja_if PUBLIC CapnProto::capnp)
target_include_directories(naja_if PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(CTest)
include(GoogleTest)
enable_testing()
add_subdirectory(test)