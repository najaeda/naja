// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <Python.h>
#include <stddef.h>
#include <stdint.h>

#define NAJA_PYTHON_RUNTIME_API_VERSION 1u
#define NAJA_PYTHON_RUNTIME_CAPSULE "najaeda.naja._C_API"

/* Append-only, versioned C interface to the importing NajaEDA runtime.
 * All calls require the GIL (including free-threaded builds' compatibility
 * GIL). Returned native pointers are borrowed: retain the Python owner and
 * prevent destruction/mutation for the complete native operation. This API
 * does not make Naja's process-global universe thread-safe.
 */
typedef struct NajaPythonRuntimeAPI {
  uint32_t abi_version;
  size_t struct_size;
  const char* naja_version;
  const char* git_commit;
  const char* compiler;
  const char* build_id;
  /* Stable token returned by NLUniverse::getRuntimeIdentity() in the
   * provider's loaded core. A C++ consumer must compare with its own linked
   * runtime's token before using pointers; version strings are insufficient.
   */
  const void* runtime_identity;
  void* (*get_universe)(void);
  /* SNLDesign* on success. NULL with TypeError for foreign/wrong objects,
   * or ReferenceError for a wrapper whose native design was destroyed. */
  void* (*unwrap_design)(PyObject* object);
} NajaPythonRuntimeAPI;

static inline const NajaPythonRuntimeAPI* NajaPythonRuntime_Import(void) {
  const NajaPythonRuntimeAPI* api = (const NajaPythonRuntimeAPI*)
      PyCapsule_Import(NAJA_PYTHON_RUNTIME_CAPSULE, 0);
  if (api != NULL &&
      (api->abi_version != NAJA_PYTHON_RUNTIME_API_VERSION ||
       api->struct_size < sizeof(NajaPythonRuntimeAPI))) {
    PyErr_SetString(PyExc_ImportError, "Incompatible NajaEDA native runtime API");
    return NULL;
  }
  return api;
}
