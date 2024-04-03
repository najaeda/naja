# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  NajaEmbeddings.cpp
)

add_library(naja_embeddings SHARED ${SOURCES})

target_link_libraries(naja_embeddings PUBLIC naja_dnl)
target_compile_options(naja_embeddings PRIVATE ${NAJA_CXX_WARNINGS}) 

#target_include_directories(naja_dnl SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
#target_include_directories(naja_dnl SYSTEM BEFORE PUBLIC ${TBB_INCLUDE_DIR})
target_include_directories(naja_embeddings PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#set_target_properties(naja_dnl PROPERTIES PUBLIC_HEADER "${DNL_HEADERS}")
#install(TARGETS naja_dnl LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 