set(SOURCES
  SNLFBS.cpp
)

add_library(naja_snl_fbs SHARED ${SOURCES})
target_link_libraries(naja_snl_fbs naja_snl)

target_include_directories(naja_snl_fbs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})