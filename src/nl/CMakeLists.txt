# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(nl)
add_subdirectory(formats)
if (NOT EMSCRIPTEN_BUILD)
  add_subdirectory(python)
endif()
if(NOT BUILD_NAJA_PYTHON AND NOT EMSCRIPTEN_BUILD)
  add_subdirectory(snippets)
endif(NOT BUILD_NAJA_PYTHON AND NOT EMSCRIPTEN_BUILD)