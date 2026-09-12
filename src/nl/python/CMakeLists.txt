# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(naja_wrapping)
if(BUILD_NAJA_PYSLANG)
  # Keep this after naja_wrapping so the Naja capsule interface target exists,
  # while locating the upstream-derived pyslang adapter with third-party code.
  add_subdirectory(
    ${PROJECT_SOURCE_DIR}/thirdparty/pyslang
    ${PROJECT_BINARY_DIR}/thirdparty/pyslang)
endif()
if(NOT BUILD_NAJA_PYTHON)
  add_subdirectory(pyloader)
endif(NOT BUILD_NAJA_PYTHON)
