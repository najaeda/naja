set(SOURCES
  NajaObject.cpp
  NajaLog.cpp
  NajaUtils.cpp
  MemoryUsage.cpp
  PerfTracker.cpp
)

set(NAJA_CORE_HEADERS
  NajaObject.h
)

add_library(naja_core SHARED ${SOURCES})

target_compile_options(naja_core PRIVATE ${NAJA_CXX_WARNINGS}) 

target_include_directories(naja_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(naja_core PROPERTIES PUBLIC_HEADER "${NAJA_CORE_HEADERS}")

install(TARGETS naja_core LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include) 