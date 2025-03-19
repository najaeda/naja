// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLPyLoader.h"
#include "SNLPyEdit.h"

#include <sstream>
#include <cstdio>
#include <Python.h>
#include <frameobject.h> // Include the header for PyFrameObject

#include "NLUniverse.h"
#include "NLLibraryTruthTables.h"
#include "NLException.h"

#include "PyNLDB.h"
#include "PyNLLibrary.h"
#include "PySNLDesign.h"

namespace {

std::string getPythonError() {
  PyObject *type, *value, *traceback;
  
  PyErr_Fetch(&type, &value, &traceback);
  PyErr_NormalizeException(&type, &value, &traceback);//
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
  std::ostringstream reason;

  if (traceback != nullptr) {
    PyTracebackObject* tracebackObj = (PyTracebackObject*)traceback;

    // Walk the traceback to the last frame
    while (tracebackObj->tb_next != NULL) {
      tracebackObj = tracebackObj->tb_next;
    }

    // Extract the line number and filename
    PyFrameObject* frame = tracebackObj->tb_frame;
    int line = PyFrame_GetLineNumber(frame);
    PyCodeObject* code = PyFrame_GetCode(frame);
    if (code != NULL) {
      PyObject* filenameObj =
          PyObject_GetAttrString((PyObject*)code, "co_filename");
      const char* filename = PyUnicode_AsUTF8(filenameObj);

      // Create a string for a message with the line number and filename
      std::ostringstream reason;
      reason << "Error in " << filename << ":" << line;
      std::string reason_str = reason.str();
      Py_DECREF(filenameObj);
      Py_DECREF(code);
    }
  }
  std::string errorStr(cStrValue + reason.str());
  Py_DECREF(strValue);

  return errorStr;
  }

PyObject* loadModule(const std::filesystem::path& path) {
  if (not std::filesystem::exists(path)) {
    std::ostringstream reason;
    reason << path << " does not exist";
    throw naja::NL::NLException(reason.str());
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
    Py_DECREF(modulePathString);
    throw naja::NL::NLException(reason.str());
  }
  Py_DECREF(modulePathString);
  return module;
}

}

namespace naja { namespace NL {

void SNLPyLoader::loadDB(
    NLDB* db,
    const std::filesystem::path& path) {
  
  auto module = loadModule(path);

  PyObject* pyDB = PYNAJA::PyNLDB_Link(db);
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
    throw NLException(reason.str());
  }
  //Cleaning
  //Py_DECREF(modulePathString);
  Py_DECREF(module);
  Py_DECREF(pyDB);
  Py_DECREF(constructString);
  Py_Finalize();
}

void SNLPyLoader::loadPrimitives(
    NLLibrary* library,
    const std::filesystem::path& primitivesPath) {
  loadLibrary(library, primitivesPath, true);
  NLLibraryTruthTables::construct(library);
}

void SNLPyLoader::loadLibrary(
    NLLibrary* library,
    const std::filesystem::path& path,
    bool loadPrimitives) {
  if (loadPrimitives and not library->isPrimitives()) {
    std::ostringstream reason;
    reason << "Cannot construct primitives in non primitives library: "
      << library->getString();
    throw NLException(reason.str());
  }
  if (not loadPrimitives and library->isPrimitives()) {
    std::ostringstream reason;
    reason << "Cannot construct non primitives in primitives library: "
      << library->getString();
    throw NLException(reason.str()); 
  }
  
  auto module = loadModule(path);

  PyObject* pyLib = PYNAJA::PyNLLibrary_Link(library);
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
    throw NLException(reason.str());
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
    throw NLException(reason.str());
  }
  auto module = loadModule(path);

  PyObject* pyDesign = PYNAJA::PySNLDesign_Link(design);
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
    throw NLException(reason.str());
  }
  //Cleaning
  Py_DECREF(module);
  Py_DECREF(pyDesign);
  Py_DECREF(constructString);
  Py_Finalize();
}

void SNLPyEdit::edit(const std::filesystem::path& path) {
  auto module = loadModule(path);

  PyObject* editString = PyUnicode_FromString("edit");

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
    throw NLException(reason.str());
  }
  //Cleaning
  //Py_DECREF(modulePathString);
  Py_DECREF(module);
  Py_DECREF(editString);
  Py_Finalize();
}

}} // namespace NL // namespace naja
