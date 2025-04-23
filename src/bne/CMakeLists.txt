# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  BNE.cpp
  actions.cpp
  Utils.cpp
  ActionTree.cpp
)

set(NAJA_BNE_HEADERS
  BNE.h
  actions.h
  Utils.h
  ActionTree.h
)

add_library(naja_bne SHARED ${SOURCES})
target_link_libraries(naja_bne PUBLIC naja_dnl coverage_config sanitizers_config)

if(APPLE AND CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "22.0.0" AND CMAKE_SYSTEM_VERSION VERSION_LESS "23.0.0")
  # macOS 13 (Ventura) has Darwin version 22.x.x
  target_compile_options(naja_bne PRIVATE -Wno-sign-conversion)
endif()
target_compile_options(naja_bne BEFORE PRIVATE ${NAJA_CXX_WARNINGS}) 

target_include_directories(naja_bne SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_bne PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(naja_bne PUBLIC ${SPDLOG_DIR})

set_target_properties(naja_bne PROPERTIES PUBLIC_HEADER "${naja_bne_HEADERS}")

install(TARGETS naja_bne LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 
