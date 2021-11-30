set(SOURCES
  MemoryUsage.cpp
  PerfTracker.cpp
  Card.cpp
)

ADD_LIBRARY(naja_core SHARED ${SOURCES})

target_compile_options(naja_core PRIVATE ${NAJA_CXX_WARNINGS}) 

target_include_directories(naja_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
