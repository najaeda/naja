<!-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# SNLSVConstructor Coverage Notes

This document tracks deliberate coverage exclusions and targeted fallback tests
for `src/nl/formats/systemverilog/frontend/SNLSVConstructor.cpp`.

## Targeted Fallback Coverage

The test suite includes dedicated benchmarks that exercise non-happy paths that
are still expected in real designs:

- `continuous_mul_zero_supported`: multiply-by-zero path in `createMultiplyAssign`.
- `continuous_eq_ne_failure_paths_unsupported`: equality / inequality fallback
  failures for non-integral and unknown operands.
- `continuous_shift_right_unresolved_skipped`: unresolved RHS net in logical
  right shift.
- `continuous_shift_left_unknown_amount_unsupported`: unresolved dynamic shift
  amount bits.
- `seq_add_unbased_x_literal_unsupported`: unbased unsized unknown literal in
  sequential add, reaching `getConstantBit` unknown handling.
- `continuous_resolve_expression_bits_failure_paths_unsupported`: targeted
  `resolveExpressionBits` fallback failures in continuous assign for unknown
  integer constants, call-cast recursion, nested binary recursion,
  concatenation recursion, and member-access base resolution.
- Diagnostics report option paths are covered with explicit tests for:
  no-diagnostics content, non-empty diagnostics content, and empty report-path
  validation.

## Deliberate LCOV Exclusions

The lines below are marked `LCOV_EXCL_LINE` because they require internal
primitive-construction failures or structurally unreachable null conditions in
the current architecture:

- `createSubAssign`: unary gate creation failure (`return false` after
  `createUnaryGate`).
- `createSubAssign`: full-adder instantiation failure (`return false` after
  `createFAInstance`).
- `createEqualityAssign`: null XNOR output after preallocated output-net call
  to `createBinaryGate`.
- `createEqualityAssign`: reduction AND creation failure with prebuilt gate
  library.
- `createInequalityAssign`: NOT gate creation failure after successful equality
  net construction.
- `createLogicalLeftShiftAssign`: missing / zero shift width after successful
  SV elaboration (defensive guard).
- `getConstantBit`: null expression after `stripConversions` on AST expression
  references (defensive guard).
- `resolveExpressionBits`: defensive call-cast guards for malformed argument
  list and zero/non-integral width (`$signed` / `$unsigned`).
- `resolveExpressionBits`: defensive unknown-bit recheck after
  `intValue.hasUnknown()`.
- `resolveExpressionBits`: internal primitive creation failure for per-bit
  binary gate construction.
- `resolveExpressionBits`: defensive concatenation operand width guard for
  invalid/non-integral operands.
- `resolveExpressionBits`: member-access fast-path fallthrough brace after
  failed recursive value resolution, prior to generic selectable fallback.
- `resolveExpressionBits`: late dynamic indexed-range multiply-selector
  recovery after direct selector bit resolution fails. Current parser-backed
  tests either resolve these selectors before the recovery path or fail before
  the multiply-specific fallback.
- `resolveExpressionBits`: brace-only LCOV artifacts in call / binary /
  element-select / member-access blocks.
- `getDriverFailureDetails`: parse-diagnostics aggregation from
  `driver.syntaxTrees` on driver failure. With current slang behavior, tested
  driver failure modes report via `sourceLoader` / argument parsing and keep
  `syntaxTrees` empty, so parse-diagnostics formatting lines are treated as
  near-unreachable.
- `dumpDiagnosticsReport`: filesystem I/O failure branches (directory creation,
  report file creation, report file final write-check) are defensive and
  environment-dependent, so they remain excluded.
