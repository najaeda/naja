# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(core)
add_subdirectory(nl)
if (NOT BUILD_EMSCRIPTEN)
  add_subdirectory(dnl)
  add_subdirectory(bne)
  add_subdirectory(optimization)
  add_subdirectory(metrics)
  add_subdirectory(najaeda)
endif()
if(NOT BUILD_NAJA_PYTHON AND NOT BUILD_EMSCRIPTEN)
  add_subdirectory(apps)
endif()