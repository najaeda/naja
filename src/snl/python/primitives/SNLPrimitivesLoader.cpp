/*
 * Copyright 2023 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SNLPrimitivesLoader.h"

#include <sstream>
#include <cstdio>
#include <Python.h>

#include "SNLUniverse.h"
#include "SNLException.h"

#include "PySNLLibrary.h"

namespace {

std::string getPythonError() {
  PyObject *type, *value, *traceback;
  PyErr_Fetch(&type, &value, &traceback);
  PyErr_Clear();
  if (value == nullptr) {
    return std::string();
  }
  PyObject* strValue = PyObject_Str(value);
  Py_DECREF(value);
  if (strValue == nullptr) {
    return std::string();
  }
  const char* cStrValue = PyUnicode_AsUTF8(strValue);
  if (cStrValue == nullptr) {
    return std::string();
  }
  std::string errorStr(cStrValue);
  Py_DECREF(strValue);
  return errorStr;
}

}

namespace naja { namespace SNL {

void SNLPrimitivesLoader::load(
    SNLLibrary* library,
    const std::filesystem::path& primitivesPath) {
  if (not library->isPrimitives()) {
    std::ostringstream reason;
    reason << "Cannot construct primitives in non primitives library: "
      << library->getString();
    throw SNLException(reason.str());
  }
  if (not std::filesystem::exists(primitivesPath)) {
    std::ostringstream reason;
    reason << primitivesPath << " does not exist";
    throw SNLException(reason.str());
  }
  auto primitivesAbsolutePath = std::filesystem::canonical(primitivesPath);

  auto moduleName = primitivesAbsolutePath.filename();
  auto modulePath = primitivesAbsolutePath.parent_path();
  moduleName.replace_extension();
  Py_Initialize();
  PyObject* sysPath = PySys_GetObject("path");
  PyObject* modulePathString = PyUnicode_FromString(modulePath.c_str());
  PyList_Append(sysPath, modulePathString);
  PyObject* primitivesModule = PyImport_ImportModule(moduleName.c_str());
  if (not primitivesModule) {
    std::ostringstream reason;
    reason << "Cannot load Python module " << primitivesAbsolutePath.string();
    std::string pythonError = getPythonError();
    if (not pythonError.empty()) {
      reason << ": " << pythonError;
    } else {
      reason << ": empty error message";
    }
    throw SNLException(reason.str());
  }

  PyObject* pyLib = PYSNL::PySNLLibrary_Link(library);
  PyObject* constructPrimitivesString = PyUnicode_FromString("constructPrimitives");
  PyObject* res =
    PyObject_CallMethodObjArgs(primitivesModule, constructPrimitivesString, pyLib, NULL);
  if (not res) {
    std::ostringstream reason;
    reason << "Error while calling constructPrimitives";
    std::string pythonError = getPythonError();
    if (not pythonError.empty()) {
      reason << ": " << pythonError;
    } else {
      reason << ": empty error message";
    }
    //Cleaning
    Py_DECREF(modulePathString);
    Py_DECREF(primitivesModule);
    Py_DECREF(pyLib);
    Py_DECREF(constructPrimitivesString);
    Py_Finalize();
    throw SNLException(reason.str());
  }
  //Cleaning
  Py_DECREF(modulePathString);
  Py_DECREF(primitivesModule);
  Py_DECREF(pyLib);
  Py_DECREF(constructPrimitivesString);
  Py_Finalize();
}

}} // namespace SNL // namespace naja