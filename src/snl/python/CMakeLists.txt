# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(snl_wrapping)
if(NOT BUILD_NAJA_PYTHON)
  add_subdirectory(pyloader)
endif(NOT BUILD_NAJA_PYTHON)
