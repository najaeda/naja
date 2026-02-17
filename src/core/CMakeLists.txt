# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

##generate Version.h
#in case Git is not available, we default to "unknown"
set(NAJA_GIT_HASH "unknown")

# find Git and if available set GIT_HASH variable
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
    OUTPUT_VARIABLE NAJA_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    )
endif(GIT_FOUND)

message(STATUS "Git hash is ${NAJA_GIT_HASH}")

configure_file(
  NajaVersion.h.in
  NajaVersion.h
  @ONLY
)

set(SOURCES
  NajaObject.cpp
  NajaUtils.cpp
  NajaProperty.cpp
  NajaPrivateProperty.cpp
  NajaDumpableProperty.cpp
  NajaPerf.cpp
  NajaLog.cpp
)

set(NAJA_CORE_HEADERS
  NajaObject.h
  NajaCollection.h
  NajaProperty.h NajaPrivateProperty.h
  NajaDumpableProperty.h
  NajaException.h
  NajaPerf.h
  NajaLog.h
)

list(APPEND NAJA_CORE_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/NajaVersion.h)

add_library(naja_core STATIC ${SOURCES})
set_property(TARGET naja_core PROPERTY POSITION_INDEPENDENT_CODE ON)
#target_link_libraries(naja_core PUBLIC cpptrace::cpptrace coverage_config sanitizers_config)
target_link_libraries(naja_core PUBLIC coverage_config sanitizers_config)
if(BUILD_NAJA_PYTHON)
  target_compile_definitions(naja_core PUBLIC SPDLOG_FMT_EXTERNAL)
endif()

target_compile_options(naja_core PRIVATE ${NAJA_CXX_WARNINGS}) 
if(MSVC)
  target_compile_options(naja_core PUBLIC /utf-8)
endif()

target_include_directories(naja_core SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)
target_include_directories(naja_core PUBLIC ${SPDLOG_DIR})

if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_core PROPERTIES PUBLIC_HEADER "${NAJA_CORE_HEADERS}")
  install(TARGETS naja_core
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}) 
endif(NOT BUILD_NAJA_PYTHON)
