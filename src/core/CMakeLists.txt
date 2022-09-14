set(SOURCES
  NajaObject.cpp
  NajaLog.cpp
  NajaUtils.cpp
  NajaProperty.cpp
  NajaPrivateProperty.cpp
  NajaDumpableProperty.cpp
  #MemoryUsage.cpp
  #PerfTracker.cpp
)

set(NAJA_CORE_HEADERS
  NajaObject.h
  NajaCollection.h
  NajaProperty.h NajaPrivateProperty.h
)

add_library(naja_core SHARED ${SOURCES})
target_link_libraries(naja_core PUBLIC coverage_config sanitizers_config)

target_compile_options(naja_core PRIVATE ${NAJA_CXX_WARNINGS}) 

target_include_directories(naja_core SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(naja_core PROPERTIES PUBLIC_HEADER "${NAJA_CORE_HEADERS}")

install(TARGETS naja_core LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 
