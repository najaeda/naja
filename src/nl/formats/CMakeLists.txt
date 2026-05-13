# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(liberty)
add_subdirectory(verilog)
add_subdirectory(systemverilog)
IF (NOT BUILD_NAJA_PYTHON)
  add_subdirectory(lefdef)
ENDIF()
