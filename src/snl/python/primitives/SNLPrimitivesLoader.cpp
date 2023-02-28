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
  auto moduleName = primitivesPath.filename();
  auto modulePath = primitivesPath.parent_path();
  //if (not std::filesystem::exists(primitivesPath)) {
  //  std::ostringstream reason;
  //  reason << primitivesPath << " does not exist";
  //  throw SNLException(reason.str());
  //}
  Py_Initialize();
  PyObject* sysPath = PySys_GetObject("path");
  PyList_Append(sysPath, PyUnicode_FromString(modulePath.c_str()));
  PyObject* primitivesModule = PyImport_ImportModule(moduleName.c_str());
  if (not primitivesModule) {
    std::ostringstream reason;
    reason << "Cannot load Python module: " << primitivesPath.string();
    throw SNLException(reason.str());
  }
  //PyObject* primivitesConstructionFunction =
  //  PyObject_GetAttrString(primitivesModule, "constructPrimitives");

  PyObject* pyLib = PYSNL::PySNLLibrary_Link(library);
  PyObject* res =
    PyObject_CallMethodOneArg(primitivesModule, PyUnicode_FromString("constructPrimitives"), pyLib);
  if (not res) {
    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    const char *pStrErrorMessage = PyUnicode_AsUTF8(pvalue);

    std::ostringstream reason;
    reason << "Error while calling constructPrimitives: " << pStrErrorMessage;
    throw SNLException(reason.str());
  }




  //auto primitivesFile = std::fopen(primitivesPath.c_str(), "r");
  //if (not primitivesFile) {
  //  throw SNLException("");
  //}

  //int ret = PyRun_SimpleFile(primitivesFile, primitivesPath.c_str());
  //if (ret != 0) {
  //  throw SNLException("");
  //}

  Py_Finalize();
}

}} // namespace SNL // namespace naja