# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_executable(naja_edit NajaEdit.cpp)
target_include_directories(naja_edit SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_edit PUBLIC ${ARGPARSE_DIR})
target_link_libraries(naja_edit
    naja_snl_pyloader
    naja_dnl naja_opt)
install(TARGETS naja_edit DESTINATION ${CMAKE_INSTALL_BINDIR})

if(BUILD_TESTING)
  foreach(TEST_CASE
      invalid-input-format
      missing-required-input
      missing-input-path
      invalid-output-format)
    add_test(
      NAME naja-edit-${TEST_CASE}
      COMMAND
        ${CMAKE_COMMAND}
        -DNAJA_EDIT=$<TARGET_FILE:naja_edit>
        -DTEST_CASE=${TEST_CASE}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/NajaEditErrorTest.cmake)
  endforeach()
endif()
