# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(DNL_SOURCES
  DNL.cpp
)
set(DNL_HEADERS
  DNL.h DNL_impl.h
)

add_library(naja_dnl SHARED ${DNL_SOURCES})

target_link_libraries(naja_dnl PUBLIC naja_nl TBB::tbb TBB::tbbmalloc)
target_compile_options(naja_dnl PRIVATE ${NAJA_CXX_WARNINGS}) 
if(APPLE AND CMAKE_SYSTEM_VERSION VERSION_EQUAL "22.0.0")
  # macOS 13 (Ventura) has Darwin version 22.x.x
  target_compile_options(naja_dnl PRIVATE -Wno-sign-conversion)
endif()
#target_compile_definitions(naja_dnl PRIVATE SNL_DESTROY_DEBUG)

target_include_directories(naja_dnl SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_dnl SYSTEM BEFORE PUBLIC ${TBB_INCLUDE_DIR})
target_include_directories(naja_dnl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(naja_dnl PROPERTIES PUBLIC_HEADER "${DNL_HEADERS}")

install(TARGETS naja_dnl
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
