# SPDX-FileCopyrightText: 2024 The Naja EDA authors <https://github.com/najaeda/naja-liberty/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(src)
if (NOT BUILD_EMSCRIPTEN)
  add_subdirectory(test)
endif(NOT BUILD_EMSCRIPTEN)
