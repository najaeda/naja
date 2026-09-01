# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

if(TEST_CASE STREQUAL "invalid-input-format")
  set(TEST_ARGS --from_format json)
  set(EXPECTED
    "Invalid value 'json' for --from_format \\(-f\\).*Expected one of: verilog, systemverilog \\(or sv\\), snl")
elseif(TEST_CASE STREQUAL "missing-required-input")
  set(TEST_ARGS --from_format snl)
  set(EXPECTED "--input \\(-i\\) is required with --from_format 'snl'")
elseif(TEST_CASE STREQUAL "missing-input-path")
  set(TEST_ARGS
    --from_format verilog
    --input definitely-missing-naja-edit-input.v)
  set(EXPECTED
    "input path 'definitely-missing-naja-edit-input.v' does not exist.*resolved to")
elseif(TEST_CASE STREQUAL "invalid-output-format")
  set(TEST_ARGS --to_format systemverilog)
  set(EXPECTED
    "Invalid value 'systemverilog' for --to_format \\(-t\\).*Expected one of: verilog, snl, dot")
else()
  message(FATAL_ERROR "Unknown naja_edit error test case: ${TEST_CASE}")
endif()

execute_process(
  COMMAND "${NAJA_EDIT}" ${TEST_ARGS}
  RESULT_VARIABLE RESULT
  OUTPUT_VARIABLE STDOUT
  ERROR_VARIABLE STDERR)
set(OUTPUT "${STDOUT}\n${STDERR}")

if(RESULT EQUAL 0)
  message(FATAL_ERROR
    "naja_edit unexpectedly accepted invalid arguments for ${TEST_CASE}:\n${OUTPUT}")
endif()
if(NOT OUTPUT MATCHES "${EXPECTED}")
  message(FATAL_ERROR
    "naja_edit did not emit the expected diagnostic for ${TEST_CASE}.\n"
    "Expected regex: ${EXPECTED}\n"
    "Output:\n${OUTPUT}")
endif()
