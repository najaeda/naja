# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

if(BUILD_NAJA_PYTHON)
  # Ship the headers matching this wheel's libraries so native consumers can
  # link the installed runtime instead of building a second copy of Naja.
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/
    DESTINATION najaeda/sdk/include/naja
    FILES_MATCHING PATTERN "*.h")
  install(FILES ${PROJECT_BINARY_DIR}/src/core/NajaVersion.h
    DESTINATION najaeda/sdk/include/naja/core)
  install(FILES
    ${PROJECT_SOURCE_DIR}/thirdparty/naja-verilog/src/VerilogConstructor.h
    ${PROJECT_SOURCE_DIR}/thirdparty/naja-verilog/src/VerilogTypes.h
    DESTINATION najaeda/sdk/include/naja/verilog)
  install(DIRECTORY ${SPDLOG_DIR}/spdlog ${ARGPARSE_DIR}/argparse
    DESTINATION najaeda/sdk/include)
  if(WIN32)
    foreach(runtime IN ITEMS naja_nl naja_dnl naja_bne naja_opt naja_metrics naja_python)
      install(FILES "$<TARGET_LINKER_FILE:${runtime}>" DESTINATION najaeda/sdk/lib)
    endforeach()
  endif()
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
