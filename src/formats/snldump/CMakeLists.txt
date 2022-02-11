set(SOURCES SNLDump.cpp)

set(HEADERS SNLDump.h)

add_library(naja_snl_dump SHARED ${SOURCES})
target_link_libraries(naja_snl_dump naja_snl coverage_config)

target_compile_options(naja_snl_dump PRIVATE ${NAJA_CXX_WARNINGS}) 

target_include_directories(naja_snl_dump PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(naja_snl_dump PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(TARGETS naja_snl_dump LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 
