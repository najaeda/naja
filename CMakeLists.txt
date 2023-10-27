cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.21)

project(naja
  VERSION 0.1.0
  HOMEPAGE_URL https://github.com/xtofalex/naja
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
#fetch argparse v2.9
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG 997da9255618311d1fcb0135ce86022729d1f1cb
)
FetchContent_MakeAvailable(argparse)

set(NAJA_CXX_WARNINGS -Werror -Wall -pedantic-errors -Wconversion -Wsign-conversion -Wextra -Wno-unused-parameter)

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

find_package(Boost REQUIRED)
find_package(Python3 COMPONENTS Development Interpreter REQUIRED)
find_package(CapnProto CONFIG REQUIRED)

add_subdirectory(thirdparty)
add_subdirectory(src)
add_subdirectory(cmake)

if(PROJECT_IS_TOP_LEVEL)
include(CTest)
enable_testing()
add_subdirectory(test)

# Doxygen
# look for Doxygen package
find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_subdirectory(docs)
endif(DOXYGEN_FOUND)
endif(PROJECT_IS_TOP_LEVEL)