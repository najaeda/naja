# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  RemoveLoadlessLogic.cpp
  ConstantPropagation.cpp
  Reduction.cpp
)

set(NAJA_OPT_HEADERS
  RemoveLoadlessLogic.h
  ConstantPropagation.h
  Reduction.h
)

add_library(naja_opt SHARED ${SOURCES})
target_link_libraries(naja_opt PUBLIC naja_bne naja_dnl coverage_config sanitizers_config)

if(APPLE AND CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "22.0.0" AND CMAKE_SYSTEM_VERSION VERSION_LESS "23.0.0")
  # see dnl compile for details
  target_compile_options(naja_opt PRIVATE -Wno-sign-conversion)
endif()
target_compile_options(naja_opt BEFORE PRIVATE ${NAJA_CXX_WARNINGS}) 

target_include_directories(naja_opt SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_opt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(naja_opt PUBLIC ${SPDLOG_DIR})

set_target_properties(naja_opt PROPERTIES PUBLIC_HEADER "${naja_opt_HEADERS}")

install(TARGETS naja_opt
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
