# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_library(naja_nl_frontend_common STATIC
  frontend/SNLDesignBuilder.cpp)
set_property(TARGET naja_nl_frontend_common PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(naja_nl_frontend_common PUBLIC naja_nl)
target_include_directories(naja_nl_frontend_common PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/frontend)

if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_nl_frontend_common PROPERTIES
    PUBLIC_HEADER "frontend/SNLDesignBuilder.h")
  install(TARGETS naja_nl_frontend_common
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
