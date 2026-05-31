# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

include(GoogleTest)

option(ENABLE_TEST_DOT
  "Enable Graphviz dot rendering in unit tests"
  OFF)

if(ENABLE_TEST_DOT)
  find_program(TEST_DOT_EXECUTABLE dot)
  if(NOT TEST_DOT_EXECUTABLE)
    message(WARNING
      "ENABLE_TEST_DOT is ON but Graphviz 'dot' was not found; "
      "dot rendering in unit tests is disabled.")
    set(ENABLE_TEST_DOT OFF)
  endif()
endif()

add_subdirectory(core)
add_subdirectory(nl)
if (NOT BUILD_EMSCRIPTEN)
  add_subdirectory(najaeda)
endif()
if(NOT BUILD_NAJA_PYTHON AND NOT BUILD_EMSCRIPTEN)
  add_subdirectory(dnl)
  add_subdirectory(bne)
  add_subdirectory(logic_opt)
  add_subdirectory(metrics)
endif()
