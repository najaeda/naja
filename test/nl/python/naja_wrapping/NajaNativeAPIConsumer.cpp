// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "NajaPythonAPI.h"
#include "NajaRuntime.h"

namespace {

const NajaPythonAPI_v1* nativeAPI = nullptr;

bool importNativeAPI() {
  PyObject* provider = PyImport_ImportModule("naja");
  if (!provider) {
    return false;
  }
  PyObject* capsule = PyObject_GetAttrString(provider, "_C_API");
  Py_DECREF(provider);
  if (!capsule) {
    return false;
  }
  auto* candidate = static_cast<const NajaPythonAPI_v1*>(
    PyCapsule_GetPointer(capsule, NAJA_PYTHON_API_CAPSULE_NAME));
  Py_DECREF(capsule);
  if (!candidate) {
    return false;
  }

  const auto status = NajaPythonAPI_ValidateV1(
    candidate,
    Naja_GetNativeBuildID(),
    Naja_GetRuntimeIdentity());
  if (status != NAJA_NATIVE_API_OK) {
    PyErr_Format(
      PyExc_ImportError,
      "incompatible Naja runtime: %s",
      NajaNativeAPI_StatusString(status));
    return false;
  }
  nativeAPI = candidate;
  return true;
}

PyObject* roundTripDesign(PyObject*, PyObject* object) {
  void* design = nullptr;
  if (nativeAPI->unwrap_snl_design(object, &design) < 0) {
    return nullptr;
  }
  return nativeAPI->wrap_nl_object(design);
}

PyObject* roundTripObject(PyObject*, PyObject* object) {
  void* nativeObject = nullptr;
  if (nativeAPI->unwrap_nl_object(object, &nativeObject) < 0) {
    return nullptr;
  }
  return nativeAPI->wrap_nl_object(nativeObject);
}

PyMethodDef methods[] = {
  {"round_trip_design", roundTripDesign, METH_O,
    "Round-trip an SNLDesign through the private Naja native API."},
  {"round_trip_object", roundTripObject, METH_O,
    "Round-trip an SNL object through the private Naja native API."},
  {nullptr, nullptr, 0, nullptr}
};

PyModuleDef module = {
  PyModuleDef_HEAD_INIT,
  "naja_native_api_consumer",
  "Test-only external consumer of the Naja native API.",
  -1,
  methods
};

}  // namespace

PyMODINIT_FUNC PyInit_naja_native_api_consumer(void) {
  if (!importNativeAPI()) {
    return nullptr;
  }
  return PyModule_Create(&module);
}

