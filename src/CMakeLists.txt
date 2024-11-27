# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(core)
add_subdirectory(snl)
add_subdirectory(najaeda)
if(NOT BUILD_NAJA_PYTHON)
  add_subdirectory(dnl)
  add_subdirectory(bne)
  add_subdirectory(apps)
  add_subdirectory(optimization)
  add_subdirectory(python)
endif(NOT BUILD_NAJA_PYTHON)