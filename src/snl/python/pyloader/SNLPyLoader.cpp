// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLPyLoader.h"
#include "SNLPyEdit.h"

#include <sstream>
#include <cstdio>
#include <Python.h>

#include "SNLUniverse.h"
#include "SNLException.h"

#include "PySNLDB.h"
#include "PySNLLibrary.h"
#include "PySNLDesign.h"

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

PyObject* loadModule(const std::filesystem::path& path) {
  if (not std::filesystem::exists(path)) {
    std::ostringstream reason;
    reason << path << " does not exist";
    throw naja::SNL::SNLException(reason.str());
  }
  auto absolutePath = std::filesystem::canonical(path);

  auto moduleName = absolutePath.filename();
  auto modulePath = absolutePath.parent_path();
  moduleName.replace_extension();
  Py_Initialize();
  PyObject* sysPath = PySys_GetObject("path");
  PyObject* modulePathString = PyUnicode_FromString(modulePath.c_str());
  PyList_Append(sysPath, modulePathString);
  PyObject* module = PyImport_ImportModule(moduleName.c_str());
  if (not module) {
    std::ostringstream reason;
    reason << "Cannot load Python module " << absolutePath.string();
    std::string pythonError = getPythonError();
    if (not pythonError.empty()) {
      reason << ": " << pythonError;
    } else {
      reason << ": empty error message";
    }
    throw naja::SNL::SNLException(reason.str());
  }
  Py_DECREF(modulePathString);
  return module;
}

}

namespace naja { namespace SNL {

void SNLPyLoader::loadDB(
    SNLDB* db,
    const std::filesystem::path& path) {
  
  auto module = loadModule(path);

  PyObject* pyDB = PYSNL::PySNLDB_Link(db);
  PyObject* constructString = PyUnicode_FromString("constructDB");

  PyObject* res =
    PyObject_CallMethodObjArgs(module, constructString, pyDB, NULL);
  if (not res) {
    std::ostringstream reason;
    reason << "Error while calling constructDB";
    std::string pythonError = getPythonError();
    if (not pythonError.empty()) {
      reason << ": " << pythonError;
    } else {
      reason << ": empty error message";
    }
    //Cleaning
    //Py_DECREF(modulePathString);
    Py_DECREF(module);
    Py_DECREF(pyDB);
    Py_DECREF(constructString);
    Py_Finalize();
    throw SNLException(reason.str());
  }
  //Cleaning
  //Py_DECREF(modulePathString);
  Py_DECREF(module);
  Py_DECREF(pyDB);
  Py_DECREF(constructString);
  Py_Finalize();
}

void SNLPyLoader::loadPrimitives(
    SNLLibrary* library,
    const std::filesystem::path& primitivesPath) {
  loadLibrary(library, primitivesPath, true);
}

void SNLPyLoader::loadLibrary(
    SNLLibrary* library,
    const std::filesystem::path& path,
    bool loadPrimitives) {
  if (loadPrimitives and not library->isPrimitives()) {
    std::ostringstream reason;
    reason << "Cannot construct primitives in non primitives library: "
      << library->getString();
    throw SNLException(reason.str());
  }
  if (not loadPrimitives and library->isPrimitives()) {
    std::ostringstream reason;
    reason << "Cannot construct non primitives in primitives library: "
      << library->getString();
    throw SNLException(reason.str()); 
  }
  
  auto module = loadModule(path);

  PyObject* pyLib = PYSNL::PySNLLibrary_Link(library);
  PyObject* constructString = nullptr;
  if (loadPrimitives) {
    constructString = PyUnicode_FromString("constructPrimitives");
  } else {
    constructString = PyUnicode_FromString("constructLibrary");
  }

  PyObject* res =
    PyObject_CallMethodObjArgs(module, constructString, pyLib, NULL);
  if (not res) {
    std::ostringstream reason;
    reason << "Error while calling construct";
    std::string pythonError = getPythonError();
    if (not pythonError.empty()) {
      reason << ": " << pythonError;
    } else {
      reason << ": empty error message";
    }
    //Cleaning
    //Py_DECREF(modulePathString);
    Py_DECREF(module);
    Py_DECREF(pyLib);
    Py_DECREF(constructString);
    Py_Finalize();
    throw SNLException(reason.str());
  }
  //Cleaning
  //Py_DECREF(modulePathString);
  Py_DECREF(module);
  Py_DECREF(pyLib);
  Py_DECREF(constructString);
  Py_Finalize();
}

void SNLPyLoader::loadDesign(
    SNLDesign* design,
    const std::filesystem::path& path) {
  if (design->isPrimitive()) {
    std::ostringstream reason;
    reason << "Cannot construct design if it is a primitive";
    throw SNLException(reason.str());
  }
  auto module = loadModule(path);

  PyObject* pyDesign = PYSNL::PySNLDesign_Link(design);
  PyObject* constructString = PyUnicode_FromString("construct");

  PyObject* res =
    PyObject_CallMethodObjArgs(module, constructString, pyDesign, NULL);
  if (not res) {
    std::ostringstream reason;
    reason << "Error while calling construct";
    std::string pythonError = getPythonError();
    if (not pythonError.empty()) {
      reason << ": " << pythonError;
    } else {
      reason << ": empty error message";
    }
    //Cleaning
    Py_DECREF(module);
    Py_DECREF(pyDesign);
    Py_DECREF(constructString);
    Py_Finalize();
    throw SNLException(reason.str());
  }
  //Cleaning
  Py_DECREF(module);
  Py_DECREF(pyDesign);
  Py_DECREF(constructString);
  Py_Finalize();
}

void SNLPyEdit::edit(const std::filesystem::path& path) {
  auto module = loadModule(path);

  PyObject* editString = editString = PyUnicode_FromString("edit");

  PyObject* res =
    PyObject_CallMethodNoArgs(module, editString);
  if (not res) {
    std::ostringstream reason;
    reason << "Error while calling edit";
    std::string pythonError = getPythonError();
    if (not pythonError.empty()) {
      reason << ": " << pythonError;
    } else {
      reason << ": empty error message";
    }
    //Cleaning
    //Py_DECREF(modulePathString);
    Py_DECREF(module);
    Py_DECREF(editString);
    Py_Finalize();
    throw SNLException(reason.str());
  }
  //Cleaning
  //Py_DECREF(modulePathString);
  Py_DECREF(module);
  Py_DECREF(editString);
  Py_Finalize();
}

}} // namespace SNL // namespace naja