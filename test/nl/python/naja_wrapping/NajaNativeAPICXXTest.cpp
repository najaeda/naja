// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <type_traits>

#include "NajaPyslangAPI.h"
#include "NajaPythonAPI.h"

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (false)

namespace {

PyObject* wrapNLObject(void* object) {
  return static_cast<PyObject*>(object);
}

int unwrapObject(PyObject* object, void** outObject) {
  *outObject = object;
  return 0;
}

PyObject* wrapCompilation(void* compilation, PyObject*) {
  return static_cast<PyObject*>(compilation);
}

PyObject* wrapSymbol(const void* symbol, PyObject*) {
  return static_cast<PyObject*>(const_cast<void*>(symbol));
}

int unwrapSymbol(PyObject* object, const void** outSymbol) {
  *outSymbol = object;
  return 0;
}

}  // namespace

int main() {
  static_assert(std::is_standard_layout_v<NajaPythonAPI_v1>);
  static_assert(std::is_standard_layout_v<NajaPyslangAPI_v1>);

  static const int runtimeToken = 0;
  const NajaPythonAPI_v1 najaAPI {
    NAJA_PYTHON_API_VERSION,
    sizeof(NajaPythonAPI_v1),
    "naja-test-build",
    &runtimeToken,
    wrapNLObject,
    unwrapObject,
    unwrapObject
  };
  CHECK(NajaPythonAPI_ValidateV1(
    &najaAPI, "naja-test-build", &runtimeToken) == NAJA_NATIVE_API_OK);

  const NajaPyslangAPI_v1 pyslangAPI {
    NAJA_PYSLANG_API_VERSION,
    sizeof(NajaPyslangAPI_v1),
    "slang-test-build",
    &runtimeToken,
    wrapCompilation,
    wrapSymbol,
    unwrapSymbol
  };
  CHECK(NajaPyslangAPI_ValidateV1(
    &pyslangAPI, "slang-test-build", &runtimeToken) ==
    NAJA_NATIVE_API_OK);
  return 0;
}
