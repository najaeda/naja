# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_library(naja_snl_pyloader SHARED SNLPyLoader.cpp)
target_link_libraries(naja_snl_pyloader PUBLIC naja_snl PRIVATE naja_snl_python Python3::Python)

target_include_directories(naja_snl_pyloader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(NAJA_SNL_PY_HEADERS SNLPyLoader.h SNLPyEdit.h)
set_target_properties(naja_snl_pyloader PROPERTIES PUBLIC_HEADER "${NAJA_SNL_PY_HEADERS}") 

install(TARGETS naja_snl_pyloader
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})