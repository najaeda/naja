set(SOURCES SNLNetlists.cpp)

add_library(snl_netlists SHARED ${SOURCES})

target_link_libraries(snl_netlists naja_snl)

target_include_directories(snl_netlists PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})