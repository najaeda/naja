# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(kernel)
if (NOT BUILD_EMSCRIPTEN)
  add_subdirectory(serialization)
endif()
add_subdirectory(decorators)
