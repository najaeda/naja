// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Result of validating a private native Python interoperability table.
///
/// The values are part of the C ABI. Add new values at the end; do not reorder
/// existing values.
typedef enum NajaNativeAPIStatus {
  NAJA_NATIVE_API_OK = 0,
  NAJA_NATIVE_API_NULL_TABLE = 1,
  NAJA_NATIVE_API_UNSUPPORTED_VERSION = 2,
  NAJA_NATIVE_API_TABLE_TOO_SMALL = 3,
  NAJA_NATIVE_API_MISSING_BUILD_ID = 4,
  NAJA_NATIVE_API_EXPECTED_BUILD_ID_REQUIRED = 5,
  NAJA_NATIVE_API_BUILD_ID_MISMATCH = 6,
  NAJA_NATIVE_API_MISSING_RUNTIME_IDENTITY = 7,
  NAJA_NATIVE_API_EXPECTED_RUNTIME_IDENTITY_REQUIRED = 8,
  NAJA_NATIVE_API_RUNTIME_IDENTITY_MISMATCH = 9,
  NAJA_NATIVE_API_MISSING_FUNCTION = 10
} NajaNativeAPIStatus;

/// Return a stable diagnostic name for a validation result.
static inline const char* NajaNativeAPI_StatusString(NajaNativeAPIStatus status) {
  switch (status) {
    case NAJA_NATIVE_API_OK:
      return "ok";
    case NAJA_NATIVE_API_NULL_TABLE:
      return "null_table";
    case NAJA_NATIVE_API_UNSUPPORTED_VERSION:
      return "unsupported_version";
    case NAJA_NATIVE_API_TABLE_TOO_SMALL:
      return "table_too_small";
    case NAJA_NATIVE_API_MISSING_BUILD_ID:
      return "missing_build_id";
    case NAJA_NATIVE_API_EXPECTED_BUILD_ID_REQUIRED:
      return "expected_build_id_required";
    case NAJA_NATIVE_API_BUILD_ID_MISMATCH:
      return "build_id_mismatch";
    case NAJA_NATIVE_API_MISSING_RUNTIME_IDENTITY:
      return "missing_runtime_identity";
    case NAJA_NATIVE_API_EXPECTED_RUNTIME_IDENTITY_REQUIRED:
      return "expected_runtime_identity_required";
    case NAJA_NATIVE_API_RUNTIME_IDENTITY_MISMATCH:
      return "runtime_identity_mismatch";
    case NAJA_NATIVE_API_MISSING_FUNCTION:
      return "missing_function";
  }
  return "unknown";
}

#ifdef __cplusplus
}
#endif

