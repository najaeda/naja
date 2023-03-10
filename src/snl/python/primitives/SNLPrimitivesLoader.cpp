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
    throw SNLException(reason.str());
  }

  PyObject* pyLib = PYSNL::PySNLLibrary_Link(library);
  PyObject* constructPrimitivesString = PyUnicode_FromString("constructPrimitives");
  PyObject* res =
    PyObject_CallMethodObjArgs(primitivesModule, constructPrimitivesString, pyLib, NULL);
  if (not res) {
    std::ostringstream reason;
    reason << "Error while calling constructPrimitives";
    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    if (pvalue) {
      PyObject *pstr = PyObject_Str(pvalue);
      if (pstr) {
        const char* err_msg = PyUnicode_AsUTF8(pstr);
        if (err_msg) {
          reason << ": " << err_msg;
        }
      }
      PyErr_Restore(ptype, pvalue, ptraceback);
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
