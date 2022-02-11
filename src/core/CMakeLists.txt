set(SOURCES
  NajaUtils.cpp
  MemoryUsage.cpp
  PerfTracker.cpp
  Card.cpp
)

add_library(naja_core SHARED ${SOURCES})

target_compile_options(naja_core PRIVATE ${NAJA_CXX_WARNINGS}) 

target_include_directories(naja_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS naja_core LIBRARY DESTINATION lib) 
