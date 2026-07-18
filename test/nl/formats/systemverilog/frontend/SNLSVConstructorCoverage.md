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
- `continuous_shift_left_unknown_amount_supported`,
  `continuous_shift_right_unknown_amount_supported`, and
  `continuous_logical_shift_right_unknown_amount_supported`: unknown literal
  shift amounts reaching the shared unknown-as-zero fallback.
- `parseContinuousShiftRightUnsupportedAmountExprReportsReason` and
  `parseContinuousShiftLeftUnsupportedAmountExprReportsGenericReason`:
  unsupported shift amount expressions reaching the shared shift-amount
  diagnostic fallback with and without a caller-provided reason string.
- `parseContinuousAssignConfigRangeCheckFunctionUnknownShiftAddressExprSupported`:
  unknown literal shift amount fallback when lowering an address expression
  inside a config range-check function call.
- `seq_add_unbased_x_literal_supported`: unbased unsized unknown literal in
  sequential add, reaching the shared unknown-as-zero arithmetic fallback.
- `continuous_sub_unknown_operand_supported`: sized unknown literal in
  continuous subtraction, reaching the shared unknown-as-zero arithmetic
  fallback.
- `continuous_resolve_expression_bits_failure_paths_unsupported`: targeted
  `resolveExpressionBits` fallback failures in continuous assign for unknown
  integer constants, call-cast recursion, nested binary recursion,
  concatenation recursion, and member-access base resolution.
- `parseCountOnesOperandResolveFailureUnsupported`: `$countones` operand
  bit-resolution failure after the operand width has been established.
- Direct inferred-memory write coverage includes constant conditional writes,
  nested packed constant-plus-dynamic selectors, two-dynamic-selector fallback,
  unresolved system-call selectors, and nested duplicate shadow commits.
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
- Active for-loop constant helpers: whitespace-only or missing source excerpts
  and parameter-initializer recursion retained for alternate AST shapes, while
  current Slang elaboration provides source ranges and folded parameter values.
- Element-selection width helpers: non-bitstream range fallbacks and missing
  fixed-range diagnostics retained for alternate type modeling; parser-backed
  dynamic element assignments reach these helpers with fixed-range bitstream
  bases.
- Inferred-memory write-target extraction: missing bitstream width after a
  supported memory signature, and dynamic-target expansion reaching the memory
  root without first collecting a dynamic selector, are defensive fallbacks.
- Inferred-memory selector guards: expansion always supplies an expression and
  selector index; after their width and resolved bits are validated, failure to
  build the equality guard requires an internal primitive-construction failure.
- Declaration-initializer DFF INIT finalization: skip after the first
  unconsumed bit of a partially consumed variable is an unordered-map iteration
  artifact, not a distinct parser-backed behavior.
- Dynamic element sub-assignment lowering: internal offset and width mismatch
  guards retained for inconsistent selection-step data after prior fixed-range
  and bit-width validation.
- `getDriverFailureDetails`: parse-diagnostics aggregation from
  `driver.syntaxTrees` on driver failure. With current slang behavior, tested
  driver failure modes report via `sourceLoader` / argument parsing and keep
  `syntaxTrees` empty, so parse-diagnostics formatting lines are treated as
  near-unreachable.
- `dumpDiagnosticsReport`: filesystem I/O failure branches (directory creation,
  report file creation, report file final write-check) are defensive and
  environment-dependent, so they remain excluded.
- Output-function `inout` / `ref` setup: the null-LHS arm after conversion is
  unreachable because output-actual collection already accepted that same call
  argument only after it converted to an LHS.
- DPI output abstraction: non-bitstream actuals are rejected before this
  lowering path, while Slang normalizes legal width conversions before the
  output bits are applied. The corresponding width and application-failure
  guards are retained as defensive checks.
