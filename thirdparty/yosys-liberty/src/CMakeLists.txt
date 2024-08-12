set(SOURCES
#  LibertyTypes.cpp
LibertyParser.cpp  
LibertyConstructor.cpp
)

set(HEADERS
#  LibertyTypes.h
#  LibertyConstructor.h
#  LibertyException.h
)

add_library(yosys_liberty STATIC ${SOURCES})
set_property(TARGET yosys_liberty PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(yosys_liberty PUBLIC coverage_config sanitizers_config)
target_include_directories(yosys_liberty
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${FLEX_INCLUDE_DIRS}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(yosys_liberty PROPERTIES PUBLIC_HEADER "${HEADERS}")
install(TARGETS yosys_liberty LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 

#add_executable(yosys_liberty_test NajaLibertySnippet.cpp)
#target_link_libraries(yosys_liberty_test yosys_liberty)
