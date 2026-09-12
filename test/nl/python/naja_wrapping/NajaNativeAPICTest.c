// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <string.h>

#include "NajaPyslangAPI.h"
#include "NajaPythonAPI.h"

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (0)

static PyObject* wrap_nl_object(void* object) {
  return (PyObject*)object;
}

static int unwrap_object(PyObject* object, void** out_object) {
  *out_object = object;
  return 0;
}

static PyObject* wrap_compilation(void* compilation, PyObject* owner) {
  (void)owner;
  return (PyObject*)compilation;
}

static PyObject* wrap_symbol(const void* symbol, PyObject* owner) {
  (void)owner;
  return (PyObject*)symbol;
}

static int unwrap_symbol(PyObject* object, const void** out_symbol) {
  *out_symbol = object;
  return 0;
}

int main(void) {
  static const int runtime_token = 0;
  static const int foreign_runtime_token = 0;

  NajaPythonAPI_v1 naja_api = {
    NAJA_PYTHON_API_VERSION,
    (uint32_t)sizeof(NajaPythonAPI_v1),
    "naja-test-build",
    &runtime_token,
    wrap_nl_object,
    unwrap_object,
    unwrap_object
  };
  CHECK(NajaPythonAPI_ValidateV1(
    &naja_api, "naja-test-build", &runtime_token) == NAJA_NATIVE_API_OK);

  naja_api.runtime_identity = &foreign_runtime_token;
  CHECK(NajaPythonAPI_ValidateV1(
    &naja_api, "naja-test-build", &runtime_token) ==
    NAJA_NATIVE_API_RUNTIME_IDENTITY_MISMATCH);
  naja_api.runtime_identity = &runtime_token;

  CHECK(NajaPythonAPI_ValidateV1(
    &naja_api, "foreign-build", &runtime_token) ==
    NAJA_NATIVE_API_BUILD_ID_MISMATCH);

  naja_api.struct_size = (uint32_t)(sizeof(NajaPythonAPI_v1) - 1u);
  CHECK(NajaPythonAPI_ValidateV1(
    &naja_api, "naja-test-build", &runtime_token) ==
    NAJA_NATIVE_API_TABLE_TOO_SMALL);

  NajaPyslangAPI_v1 pyslang_api = {
    NAJA_PYSLANG_API_VERSION,
    (uint32_t)sizeof(NajaPyslangAPI_v1),
    "slang-test-build",
    &runtime_token,
    wrap_compilation,
    wrap_symbol,
    unwrap_symbol
  };
  CHECK(NajaPyslangAPI_ValidateV1(
    &pyslang_api, "slang-test-build", &runtime_token) ==
    NAJA_NATIVE_API_OK);

  pyslang_api.unwrap_symbol = NULL;
  CHECK(NajaPyslangAPI_ValidateV1(
    &pyslang_api, "slang-test-build", &runtime_token) ==
    NAJA_NATIVE_API_MISSING_FUNCTION);

  CHECK(strcmp(
    NajaNativeAPI_StatusString(NAJA_NATIVE_API_RUNTIME_IDENTITY_MISMATCH),
    "runtime_identity_mismatch") == 0);
  return 0;
}
