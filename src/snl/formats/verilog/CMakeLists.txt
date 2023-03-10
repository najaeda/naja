set(SOURCES
  frontend/SNLVRLConstructor.cpp
  backend/SNLVRLDumper.cpp
)

SET(HEADERS
  frontend/SNLVRLConstructor.h
  backend/SNLVRLDumper.h
)

add_library(naja_snl_verilog SHARED ${SOURCES})
target_link_libraries(naja_snl_verilog naja_verilog naja_snl)

target_include_directories(naja_snl_verilog PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/frontend
  ${CMAKE_CURRENT_SOURCE_DIR}/backend)

set_target_properties(naja_snl_verilog PROPERTIES PUBLIC_HEADER "${HEADERS}")
install(TARGETS naja_snl_verilog LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 