CMAKE_POLICY(SET CMP0048 NEW) 

PROJECT(naja
  VERSION 0.1.0
  HOMEPAGE_URL https://github.com/xtofalex/naja
  LANGUAGES CXX
)

CMAKE_MINIMUM_REQUIRED(VERSION 3.19)
SET(CMAKE_CXX_STANDARD 20)
SET(NAJA_CXX_WARNINGS -Werror -Wall -pedantic-errors -Wconversion -Wsign-conversion -Wextra -Wno-unused-parameter)

ADD_LIBRARY(coverage_config INTERFACE)
OPTION(CODE_COVERAGE "Enable coverage reporting" OFF)
IF(CODE_COVERAGE)
  # Add required flags (GCC & LLVM/Clang)
  TARGET_COMPILE_OPTIONS(coverage_config INTERFACE
    -O0        # no optimization
    -g         # generate debug info
    --coverage # sets all required flags
  )
  target_link_options(coverage_config INTERFACE --coverage)
ENDIF(CODE_COVERAGE)

FIND_PACKAGE(Boost REQUIRED)
FIND_PACKAGE(LibXml2 REQUIRED)
FIND_PACKAGE(Python3 COMPONENTS Development REQUIRED)

ADD_SUBDIRECTORY(thirdparty)
ADD_SUBDIRECTORY(src)

include(CTest)
enable_testing()
add_subdirectory(test)

# Doxygen

# look for Doxygen package
find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_subdirectory(docs)
endif(DOXYGEN_FOUND)
