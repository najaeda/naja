cmake_policy(SET CMP0048 NEW)

project(naja
  VERSION 0.1.0
  HOMEPAGE_URL https://github.com/xtofalex/naja
  LANGUAGES CXX
)

cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_STANDARD 20)
set(NAJA_CXX_WARNINGS -Werror -Wall -pedantic-errors -Wconversion -Wsign-conversion -Wextra -Wno-unused-parameter)

add_library(coverage_config INTERFACE)
option(CODE_COVERAGE "Enable coverage reporting" OFF)
IF(CODE_COVERAGE)
  # Add required flags (GCC & LLVM/Clang)
  TARGET_COMPILE_OPTIONS(coverage_config INTERFACE
    -O0        # no optimization
    -g         # generate debug info
    --coverage # sets all required flags
  )
  target_link_options(coverage_config INTERFACE --coverage)
ENDIF(CODE_COVERAGE)

find_package(Boost REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Python3 COMPONENTS Development)
find_package(CapnProto CONFIG REQUIRED)

add_subdirectory(thirdparty)
add_subdirectory(src)
add_subdirectory(cmake)

include(CTest)
enable_testing()
add_subdirectory(test)

# Doxygen

# look for Doxygen package
find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_subdirectory(docs)
endif(DOXYGEN_FOUND)
