cmake_policy(SET CMP0048 NEW) 

project(naja
  VERSION 0.1.0
  HOMEPAGE_URL https://github.com/xtofalex/naja
  LANGUAGES CXX
)

cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Python3 COMPONENTS Development REQUIRED)

add_subdirectory(src)

include(CTest)
enable_testing()
add_subdirectory(test)

# Doxygen

# look for Doxygen package
find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_subdirectory(docs)
endif(DOXYGEN_FOUND)
