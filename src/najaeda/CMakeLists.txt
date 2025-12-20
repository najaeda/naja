# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

if(BUILD_NAJA_PYTHON)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/najaeda
    DESTINATION .
    PATTERN "*~" EXCLUDE
    PATTERN "__pycache__" EXCLUDE 
  )
else(BUILD_NAJA_PYTHON)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/najaeda
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/python
    PATTERN "*~" EXCLUDE
    PATTERN "__pycache__" EXCLUDE 
  )
endif(BUILD_NAJA_PYTHON)
