# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)
project(VHDLFrontend VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)

add_library(vhdl_frontend STATIC src/Lexer.cpp src/Parser.cpp)
add_library(vhdl::frontend ALIAS vhdl_frontend)
target_compile_features(vhdl_frontend PUBLIC cxx_std_20)
target_include_directories(vhdl_frontend
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(vhdl_frontend PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  EXPORT_NAME frontend
)

install(TARGETS vhdl_frontend
  EXPORT VHDLFrontendTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT VHDLFrontendTargets
  NAMESPACE vhdl::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VHDLFrontend
)

if(BUILD_TESTING)
  set(_vhdl_gtest_default "")
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/googletest/CMakeLists.txt")
    set(_vhdl_gtest_default "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/googletest")
  endif()
  set(VHDL_GTEST_SOURCE_DIR "${_vhdl_gtest_default}" CACHE PATH
      "Path to a GoogleTest source checkout; otherwise GTest must be installed")
  if(VHDL_GTEST_SOURCE_DIR)
    set(BUILD_GMOCK OFF CACHE BOOL "Build GoogleMock" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "Install GoogleTest with this project" FORCE)
    add_subdirectory("${VHDL_GTEST_SOURCE_DIR}"
                     "${CMAKE_CURRENT_BINARY_DIR}/_deps/googletest" EXCLUDE_FROM_ALL)
    set(VHDL_GTEST_MAIN_TARGET gtest_main)
  else()
    find_package(GTest CONFIG REQUIRED)
    set(VHDL_GTEST_MAIN_TARGET GTest::gtest_main)
  endif()

  include(GoogleTest)

  add_executable(vhdl_lexer_tests tests/lexer/LexerTest.cpp)
  target_link_libraries(vhdl_lexer_tests PRIVATE vhdl::frontend ${VHDL_GTEST_MAIN_TARGET})
  gtest_discover_tests(vhdl_lexer_tests)

  add_executable(vhdl_parser_tests tests/parser/ParserTest.cpp)
  target_link_libraries(vhdl_parser_tests PRIVATE vhdl::frontend ${VHDL_GTEST_MAIN_TARGET})
  gtest_discover_tests(vhdl_parser_tests)
endif()
