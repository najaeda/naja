# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(netlists)
if (NOT EMSCRIPTEN_BUILD)
  add_subdirectory(formats)
  add_subdirectory(python)
endif()
add_subdirectory(snl)
add_subdirectory(pnl)
if(NOT BUILD_NAJA_PYTHON AND NOT EMSCRIPTEN_BUILD)
  add_subdirectory(visual)
endif()
