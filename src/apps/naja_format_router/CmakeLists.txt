# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_executable(naja_format_router NajaFormatRouter.cpp)
target_include_directories(naja_format_router SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_format_router PUBLIC ${ARGPARSE_DIR})
target_link_libraries(naja_format_router
    naja_snl_pyloader
    naja_dnl naja_opt naja_lefdef)
install(TARGETS naja_format_router DESTINATION ${CMAKE_INSTALL_BINDIR})
