// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <Python.h>
#include <stdint.h>
#include <string.h>

#include "NajaNativeAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Private PyCapsule name exported as ``najaeda.naja._C_API``.
#define NAJA_PYTHON_API_CAPSULE_NAME "najaeda.naja._C_API"
#define NAJA_PYTHON_API_VERSION 1u

/// Version 1 of the native Naja Python consumer contract.
///
/// All native object pointers are opaque at this boundary. A consumer may cast
/// them to Naja C++ types only after NajaPythonAPI_ValidateV1 succeeds against
/// both the consumer's compiled build id and its directly linked Naja runtime
/// identity token.
typedef struct NajaPythonAPI_v1 {
  uint32_t api_version;
  uint32_t struct_size;
  const char* build_id;
  const void* runtime_identity;

  /// Wrap a live NLObject pointer using najaeda.naja's canonical Python type.
  /// Returns a new reference, or NULL with a Python exception set.
  PyObject* (*wrap_nl_object)(void* object);

  /// Validate and unwrap a raw najaeda.naja NLObject.
  /// Returns 0 on success and writes to out_object; otherwise returns -1 and
  /// sets a Python exception without writing a usable pointer.
  int (*unwrap_nl_object)(PyObject* object, void** out_object);

  /// Validate and unwrap a raw najaeda.naja SNLDesign.
  /// Returns 0 on success and writes to out_design; otherwise returns -1 and
  /// sets a Python exception without writing a usable pointer.
  int (*unwrap_snl_design)(PyObject* object, void** out_design);
} NajaPythonAPI_v1;

/// Validate a Naja API table without calling any function from that table.
///
/// Exact build-id matching is intentional for v1: the crossed objects are C++
/// objects and Naja does not yet promise a stable C++ ABI. Runtime identity is
/// a process-local address token and catches duplicate native runtimes even
/// when their build ids are identical.
static inline NajaNativeAPIStatus NajaPythonAPI_ValidateV1(
  const NajaPythonAPI_v1* api,
  const char* expected_build_id,
  const void* expected_runtime_identity) {
  if (api == NULL) {
    return NAJA_NATIVE_API_NULL_TABLE;
  }
  if (api->api_version != NAJA_PYTHON_API_VERSION) {
    return NAJA_NATIVE_API_UNSUPPORTED_VERSION;
  }
  if (api->struct_size < (uint32_t)sizeof(NajaPythonAPI_v1)) {
    return NAJA_NATIVE_API_TABLE_TOO_SMALL;
  }
  if (api->build_id == NULL || api->build_id[0] == '\0') {
    return NAJA_NATIVE_API_MISSING_BUILD_ID;
  }
  if (expected_build_id == NULL || expected_build_id[0] == '\0') {
    return NAJA_NATIVE_API_EXPECTED_BUILD_ID_REQUIRED;
  }
  if (strcmp(api->build_id, expected_build_id) != 0) {
    return NAJA_NATIVE_API_BUILD_ID_MISMATCH;
  }
  if (api->runtime_identity == NULL) {
    return NAJA_NATIVE_API_MISSING_RUNTIME_IDENTITY;
  }
  if (expected_runtime_identity == NULL) {
    return NAJA_NATIVE_API_EXPECTED_RUNTIME_IDENTITY_REQUIRED;
  }
  if (api->runtime_identity != expected_runtime_identity) {
    return NAJA_NATIVE_API_RUNTIME_IDENTITY_MISMATCH;
  }
  if (api->wrap_nl_object == NULL ||
      api->unwrap_nl_object == NULL ||
      api->unwrap_snl_design == NULL) {
    return NAJA_NATIVE_API_MISSING_FUNCTION;
  }
  return NAJA_NATIVE_API_OK;
}

#ifdef __cplusplus
}
#endif

