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

/// Private PyCapsule name exported as ``najaeda.pyslang._C_API``.
#define NAJA_PYSLANG_API_CAPSULE_NAME "najaeda.pyslang._C_API"
#define NAJA_PYSLANG_API_VERSION 1u

/// Version 1 of the bundled pyslang bridge used by najaeda.naja.
///
/// wrap_compilation and wrap_symbol create non-owning views. The provider must
/// retain a strong reference to owner in every returned view so the Naja
/// frontend session cannot be destroyed while the native pointer is borrowed.
typedef struct NajaPyslangAPI_v1 {
  uint32_t api_version;
  uint32_t struct_size;
  const char* build_id;
  const void* runtime_identity;

  PyObject* (*wrap_compilation)(void* compilation, PyObject* owner);
  PyObject* (*wrap_symbol)(const void* symbol, PyObject* owner);
  int (*unwrap_symbol)(PyObject* object, const void** out_symbol);
} NajaPyslangAPI_v1;

/// Validate a bundled pyslang table without calling any function from it.
static inline NajaNativeAPIStatus NajaPyslangAPI_ValidateV1(
  const NajaPyslangAPI_v1* api,
  const char* expected_build_id,
  const void* expected_runtime_identity) {
  if (api == NULL) {
    return NAJA_NATIVE_API_NULL_TABLE;
  }
  if (api->api_version != NAJA_PYSLANG_API_VERSION) {
    return NAJA_NATIVE_API_UNSUPPORTED_VERSION;
  }
  if (api->struct_size < (uint32_t)sizeof(NajaPyslangAPI_v1)) {
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
  if (api->wrap_compilation == NULL ||
      api->wrap_symbol == NULL ||
      api->unwrap_symbol == NULL) {
    return NAJA_NATIVE_API_MISSING_FUNCTION;
  }
  return NAJA_NATIVE_API_OK;
}

#ifdef __cplusplus
}
#endif

