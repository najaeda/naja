# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_custom_target(naja_python_package ALL
  COMMAND ${CMAKE_COMMAND} -E echo "Creating naja_python_package..."
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/naja ${CMAKE_CURRENT_BINARY_DIR}/naja
  #COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/output
  #COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/output/init.txt
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:snl> ${CMAKE_CURRENT_BINARY_DIR}/naja
  COMMAND ${CMAKE_COMMAND} -E echo "naja python package completed."
)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/naja
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/python
  PATTERN "*~" EXCLUDE
  PATTERN "__pycache__" EXCLUDE 
)
