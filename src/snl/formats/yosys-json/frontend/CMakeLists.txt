set(SOURCES
  SNLYosysJSONParser.cpp
)

add_library(naja_snl_yosys_json SHARED ${SOURCES})
target_link_libraries(naja_snl_yosys_json naja_snl)

target_compile_definitions(naja_snl_yosys_json PRIVATE YOSYS_JSON_PARSER_DEBUG)

target_include_directories(naja_snl_yosys_json PUBLIC ${PROJECT_SOURCE_DIR}/thirdparty)
target_include_directories(naja_snl_yosys_json PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})