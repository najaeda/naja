# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(nl)
add_subdirectory(formats)
add_subdirectory(python)
if(NOT BUILD_NAJA_PYTHON)
  add_subdirectory(snippets)
endif(NOT BUILD_NAJA_PYTHON)
