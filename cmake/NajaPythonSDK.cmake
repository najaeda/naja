# SPDX-FileCopyrightText: 2026 The Naja authors
# SPDX-License-Identifier: Apache-2.0

set(naja_sdk_build_dir "${CMAKE_CURRENT_BINARY_DIR}/sdk")
file(MAKE_DIRECTORY "${naja_sdk_build_dir}")
set(NAJA_RUNTIME_GIT_COMMIT "unknown")
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE NAJA_RUNTIME_GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
endif()
set(NAJA_RUNTIME_COMPILER "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
file(READ "${Boost_INCLUDE_DIR}/boost/version.hpp" boost_version_header)
string(REGEX MATCH "#define BOOST_VERSION ([0-9]+)" boost_version_match "${boost_version_header}")
set(NAJA_RUNTIME_BOOST_VERSION "${CMAKE_MATCH_1}")
file(READ "${PROJECT_SOURCE_DIR}/src/core/NajaVersion.h.in" version_source)
string(REGEX MATCH "NAJA_VERSION[ \t]*\\{[ \t]*\"([^\"]+)\"" version_match "${version_source}")
set(NAJA_RUNTIME_VERSION "${CMAKE_MATCH_1}")

# Include the source contents, not only HEAD: development provider builds must
# not claim ABI identity with another dirty build from the same git commit.
file(GLOB_RECURSE naja_runtime_sources CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/src/*.h" "${PROJECT_SOURCE_DIR}/src/*.cpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${naja_runtime_sources})
set(runtime_source_hashes "")
foreach(source IN LISTS naja_runtime_sources)
  file(SHA256 "${source}" source_hash)
  string(APPEND runtime_source_hashes "${source_hash}")
endforeach()
string(SHA256 NAJA_RUNTIME_BUILD_ID
  "${NAJA_RUNTIME_GIT_COMMIT}|${NAJA_RUNTIME_VERSION}|${NAJA_RUNTIME_COMPILER}|${CMAKE_SYSTEM_NAME}|${CMAKE_SYSTEM_PROCESSOR}|${CMAKE_SIZEOF_VOID_P}|${CMAKE_CXX_STANDARD}|${CMAKE_CXX_FLAGS}|${CMAKE_CXX_FLAGS_RELEASE}|${Python3_SOABI}|${NAJA_RUNTIME_BOOST_VERSION}|${TBB_VERSION}|${runtime_source_hashes}")
configure_file("${CMAKE_CURRENT_LIST_DIR}/NajaRuntimeBuild.h.in"
  "${naja_sdk_build_dir}/NajaRuntimeBuild.h" @ONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/NajaEDABuild.json.in"
  "${naja_sdk_build_dir}/build.json" @ONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/NajaEDAConfig.cmake.in"
  "${naja_sdk_build_dir}/NajaEDAConfig.cmake" @ONLY)
target_include_directories(naja PRIVATE "${naja_sdk_build_dir}")

install(FILES "${naja_sdk_build_dir}/build.json"
  DESTINATION najaeda/sdk)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/README-python-sdk.md"
  DESTINATION najaeda/sdk RENAME README.md)
install(FILES "${naja_sdk_build_dir}/NajaEDAConfig.cmake"
  DESTINATION najaeda/sdk/cmake)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/src/"
  DESTINATION najaeda/sdk/include/naja
  FILES_MATCHING PATTERN "*.h")
install(FILES
  "${PROJECT_SOURCE_DIR}/src/nl/python/naja_wrapping/NajaPythonRuntimeAPI.h"
  "${naja_sdk_build_dir}/NajaRuntimeBuild.h"
  DESTINATION najaeda/sdk/include)
install(FILES "${PROJECT_BINARY_DIR}/src/core/NajaVersion.h"
  DESTINATION najaeda/sdk/include/naja/core)
# SNLVRLConstructor's public interface derives from the Verilog parser's
# constructor API. Only those public headers are needed by runtime consumers;
# scanner and generated parser implementation headers remain private.
install(FILES
  "${PROJECT_SOURCE_DIR}/thirdparty/naja-verilog/src/VerilogConstructor.h"
  "${PROJECT_SOURCE_DIR}/thirdparty/naja-verilog/src/VerilogTypes.h"
  DESTINATION najaeda/sdk/include/naja/verilog)
install(DIRECTORY "${SPDLOG_DIR}/spdlog"
  DESTINATION najaeda/sdk/include)
install(DIRECTORY "${ARGPARSE_DIR}/argparse"
  DESTINATION najaeda/sdk/include)

# MSVC consumers need the import libraries as well as the provider DLLs.
if(WIN32)
  foreach(runtime IN ITEMS naja_nl naja_dnl naja_bne naja_opt naja_metrics naja_python)
    install(FILES "$<TARGET_LINKER_FILE:${runtime}>"
      DESTINATION najaeda/sdk/lib)
  endforeach()
endif()
