# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_executable(naja_edit NajaEdit.cpp)
target_include_directories(naja_edit SYSTEM BEFORE PRIVATE ${Boost_INCLUDE_DIR})
target_include_directories(naja_edit PRIVATE ${ARGPARSE_DIR})
target_link_libraries(naja_edit
    naja_snl_verilog naja_snl_pyloader naja_snl_liberty
    naja_dnl naja_opt naja_snl_visual)
install(TARGETS naja_edit DESTINATION ${CMAKE_INSTALL_BINDIR})
