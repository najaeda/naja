# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

# Copy najaeda package sources into the test binary directory.
file(COPY "${SRC}/" DESTINATION "${DST}")

# Symlink naja.so into the package directory (remove first to be idempotent).
set(LINK "${DST}/naja.so")
if(EXISTS "${LINK}" OR IS_SYMLINK "${LINK}")
  file(REMOVE "${LINK}")
endif()
execute_process(
  COMMAND ${CMAKE_COMMAND} -E create_symlink "${NAJA_SO}" "${LINK}"
)
