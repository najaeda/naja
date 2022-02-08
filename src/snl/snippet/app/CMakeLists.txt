cmake_policy(SET CMP0048 NEW)

project(naja_app_example)

cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)

#list(APPEND CMAKE_MODULE_PATH "${NAJA_INSTALL}/cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "$ENV{NAJA_INSTALL}/cmake")

find_package(Boost REQUIRED)
find_package(Naja REQUIRED)

add_subdirectory(src)
