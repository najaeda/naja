set(SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/frontend/SNLVRLConstructor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/backend/SNLVRLDumper.cpp
)

add_library(naja_snl_verilog SHARED ${SOURCES})
target_link_libraries(naja_snl_verilog naja_verilog naja_snl)

target_include_directories(naja_snl_verilog PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/frontend
  ${CMAKE_CURRENT_SOURCE_DIR}/backend)