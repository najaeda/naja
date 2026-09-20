# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)
project(VHDLFrontend VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CTest)

add_library(vhdl_frontend STATIC src/Lexer.cpp)
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
  add_executable(vhdl_lexer_tests tests/lexer/LexerTest.cpp)
  target_link_libraries(vhdl_lexer_tests PRIVATE vhdl::frontend)
  add_test(NAME VHDLLexerTests COMMAND vhdl_lexer_tests)
endif()
