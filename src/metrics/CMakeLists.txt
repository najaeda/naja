# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  FanoutComputer.cpp
  LogicLevelComputer.cpp
)

set(naja_metrics_HEADERS
  FanoutComputer.h
  LogicLevelComputer.h
)

add_library(naja_metrics SHARED ${SOURCES})
target_link_libraries(naja_metrics PUBLIC naja_bne naja_dnl coverage_config sanitizers_config)

if(APPLE)
  # see dnl compile for details
  target_compile_options(naja_metrics PRIVATE -Wno-sign-conversion)
endif()
target_compile_options(naja_metrics BEFORE PRIVATE ${NAJA_CXX_WARNINGS}) 

target_include_directories(naja_metrics SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_metrics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(naja_metrics PUBLIC ${SPDLOG_DIR})

if(BUILD_NAJA_PYTHON)
  install(TARGETS naja_metrics LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/najaeda)
else(BUILD_NAJA_PYTHON)
  set_target_properties(naja_metrics PROPERTIES PUBLIC_HEADER "${naja_metrics_HEADERS}")
  install(TARGETS naja_metrics
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(BUILD_NAJA_PYTHON)