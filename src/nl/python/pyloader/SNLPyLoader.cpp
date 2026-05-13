// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLPyLoader.h"
#include "SNLPyEdit.h"

#include <sstream>
#include <string>
#include <Python.h>
#include <frameobject.h>

#include "NLUniverse.h"
#include "NLLibraryTruthTables.h"
#include "NLException.h"

#include "PyNLDB.h"
#include "PyNLLibrary.h"
#include "PySNLDesign.h"

namespace {

// RAII wrapper for PyObject* — releases reference on scope exit.
struct PyObjRef {
  explicit PyObjRef(PyObject* o = nullptr) : obj(o) {}
  ~PyObjRef() { Py_XDECREF(obj); }
  PyObject* get() const { return obj; }
  explicit operator bool() const { return obj != nullptr; }
  PyObjRef(const PyObjRef&) = delete;
  PyObjRef& operator=(const PyObjRef&) = delete;
private:
  PyObject* obj;
};

// Initializes the Python interpreter if not already running.
// Finalizes it on destruction only if this instance started it.
// Uses Py_InitializeEx(0) to avoid overriding the host's signal handlers.
struct PythonInit {
  PythonInit() : owned(!Py_IsInitialized()) {
    if (owned) {
      Py_InitializeEx(0);
    }
  }
  ~PythonInit() {
    if (owned) {
      Py_Finalize();
    }
  }
  const bool owned;
};

std::string getPythonError() {
  PyObject* type     = nullptr;
  PyObject* value    = nullptr;
  PyObject* traceback = nullptr;

  PyErr_Fetch(&type, &value, &traceback);
  PyErr_NormalizeException(&type, &value, &traceback);
  PyErr_Clear();

  std::string result;

  if (value) {
    PyObjRef strValue(PyObject_Str(value));
    if (strValue) {
      const char* c = PyUnicode_AsUTF8(strValue.get());
      if (c) {
        result = c;
      }
    }
  }

  if (traceback) {
    auto* tb = reinterpret_cast<PyTracebackObject*>(traceback);
    while (tb->tb_next) {
      tb = tb->tb_next;
    }
    int line = PyFrame_GetLineNumber(tb->tb_frame);
    PyObjRef code(reinterpret_cast<PyObject*>(PyFrame_GetCode(tb->tb_frame)));
    if (code) {
      PyObjRef filenameObj(PyObject_GetAttrString(code.get(), "co_filename"));
      if (filenameObj) {
        const char* filename = PyUnicode_AsUTF8(filenameObj.get());
        if (filename) {
          result += std::string(" (") + filename + ":" + std::to_string(line) + ")";
        }
      }
    }
  }

  Py_XDECREF(type);
  Py_XDECREF(value);
  Py_XDECREF(traceback);
  return result;
}

PyObject* loadModule(const std::filesystem::path& path) {
  if (not std::filesystem::exists(path)) {
    std::ostringstream reason;
    reason << path << " does not exist";
    throw naja::NL::NLException(reason.str());
  }
  auto absolutePath = std::filesystem::canonical(path);
  auto moduleName = absolutePath.stem();
  auto modulePath = absolutePath.parent_path();

  PyObject* sysPath = PySys_GetObject("path");  // borrowed reference
  PyObjRef modulePathString(PyUnicode_FromString(modulePath.c_str()));
  PyList_Append(sysPath, modulePathString.get());

  PyObject* module = PyImport_ImportModule(moduleName.c_str());
  if (not module) {
    std::ostringstream reason;
    reason << "Cannot load Python module " << absolutePath.string();
    std::string pythonError = getPythonError();
    if (not pythonError.empty()) {
      reason << ": " << pythonError;
    }
    throw naja::NL::NLException(reason.str());
  }
  return module;
}

void callMethod(
    PyObject* module,
    const char* methodName,
    PyObject* arg,
    const std::string& context) {
  PyObjRef methodString(PyUnicode_FromString(methodName));
  PyObjRef res(PyObject_CallMethodObjArgs(module, methodString.get(), arg, NULL));
  if (not res) {
    std::string pythonError = getPythonError();
    std::ostringstream reason;
    reason << "Error while calling " << context;
    if (not pythonError.empty()) {
      reason << ": " << pythonError;
    }
    throw naja::NL::NLException(reason.str());
  }
}

}  // namespace

namespace naja::NL {

void SNLPyLoader::loadDB(
    NLDB* db,
    const std::filesystem::path& path) {
  PythonInit pyInit;
  PyObjRef module(loadModule(path));
  PyObjRef pyDB(PYNAJA::PyNLDB_Link(db));
  callMethod(module.get(), "constructDB", pyDB.get(), "constructDB");
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
  PythonInit pyInit;
  PyObjRef module(loadModule(path));
  PyObjRef pyLib(PYNAJA::PyNLLibrary_Link(library));
  const char* methodName = loadPrimitives ? "constructPrimitives" : "constructLibrary";
  callMethod(module.get(), methodName, pyLib.get(), methodName);
}

void SNLPyLoader::loadDesign(
    SNLDesign* design,
    const std::filesystem::path& path) {
  if (design->isPrimitive()) {
    throw NLException("Cannot construct design if it is a primitive");
  }
  PythonInit pyInit;
  PyObjRef module(loadModule(path));
  PyObjRef pyDesign(PYNAJA::PySNLDesign_Link(design));
  callMethod(module.get(), "construct", pyDesign.get(), "construct");
}

void SNLPyEdit::edit(const std::filesystem::path& path) {
  PythonInit pyInit;
  PyObjRef module(loadModule(path));
  PyObjRef methodString(PyUnicode_FromString("edit"));
  PyObjRef res(PyObject_CallMethodNoArgs(module.get(), methodString.get()));
  if (not res) {
    std::string pythonError = getPythonError();
    std::ostringstream reason;
    reason << "Error while calling edit";
    if (not pythonError.empty()) {
      reason << ": " << pythonError;
    }
    throw NLException(reason.str());
  }
}

}  // namespace naja::NL
