// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDB.h"

#include "PyInterface.h"
#include "PySNLLibraries.h"
#include "PySNLLibrary.h"
#include "PySNLUniverse.h"

#include "SNLDB.h"

#include <Python.h>
#include <filesystem>
#include "SNLCapnP.h"
#include "SNLLibertyConstructor.h"
#include "SNLUtils.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"

#include <fstream>
#include <iostream>

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDB, function)

static PyObject* PySNLDB_create(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.create", &arg)) {
    setError("malformed SNLDB create");
    return nullptr;
  }
  if (not IsPySNLUniverse(arg)) {
    setError("SNLDB create argument should be a SNLUniverse");
    return nullptr;
  }
  auto universe = PYSNLUNIVERSE_O(arg);
  SNLDB* db = nullptr;
  TRY db = SNLDB::create(universe);
  SNLCATCH
  return PySNLDB_Link(db);
}

static PyObject* PySNLDB_loadSNL(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.loadSNL", &arg)) {
    setError("malformed SNLDB loadSNL");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    setError("SNLDB loadSNL argument should be a file path");
    return nullptr;
  }
  SNLDB* db = nullptr;
  const std::filesystem::path path(PyUnicode_AsUTF8(arg));
  if (SNLUniverse::get() == nullptr) {
    SNLUniverse::create();
  }
  TRY db = SNLCapnP::load(path);
  SNLCATCH
  return PySNLDB_Link(db);
}

PyObject* PySNLDB_dumpSNL(PySNLDB* self, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.dumpSNL", &arg)) {
    setError("malformed SNLDB dumpSNL");
    Py_RETURN_FALSE;
  }
  if (not PyUnicode_Check(arg)) {
    setError("SNLDB dumpSNL argument should be a file path");
    Py_RETURN_FALSE;
  }
  SNLCapnP::dump(self->object_, PyUnicode_AsUTF8(arg));
  // return true to python
  Py_RETURN_TRUE;
}


PyObject* PySNLDB_loadLibertyPrimitives(PySNLDB* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.loadLibertyPrimitives", &arg0)) {
    setError("malformed SNLDB loadLibertyPrimitives");
    Py_RETURN_FALSE;
  }
  if (not PyList_Check(arg0)) {
    setError("malformed SNLDesign.loadLibertyPrimitives method");
    Py_RETURN_FALSE;
  }
  SNLDB* db = nullptr;
  SNLLibrary* primitivesLibrary = nullptr;
  db = self->object_;
  primitivesLibrary = db->getLibrary(SNLName("PRIMS"));
  if (primitivesLibrary == nullptr) {
    primitivesLibrary =
      SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  }
  for (int i = 0; i < PyList_Size(arg0); ++i) {
    PyObject* object = PyList_GetItem(arg0, i);
    if (not PyUnicode_Check(object)) {
      setError("SNLDB loadLibertyPrimitives argument should be a file path");
      Py_RETURN_FALSE;
    }
    std::string pathStr = PyUnicode_AsUTF8(object);
    const std::filesystem::path path(pathStr);

    auto extension = path.extension();
    if (extension.empty()) {
      setError("SNLDB loadLibertyPrimitives design path has no extension");
      Py_RETURN_FALSE;
    } else if (extension == ".lib") {
      SNLLibertyConstructor constructor(primitivesLibrary);
      constructor.construct(path);
    } else {
      setError("SNLDB loadLibertyPrimitives");
      Py_RETURN_FALSE;
    }
  }
  Py_RETURN_TRUE;
}

PyObject* PySNLDB_loadVerilog(PySNLDB* self, PyObject* args) {
  PyObject* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.loadVerilog", &arg1)) {
    setError("malformed SNLDB loadVerilog");
    Py_RETURN_FALSE;
  }
  if (not PyList_Check(arg1)) {
    setError("malformed SNLDesign.loadVerilog method");
    Py_RETURN_FALSE;
  }
  SNLDB* db = self->object_;
  auto designLibrary = SNLLibrary::create(db, SNLName("DESIGN"));
  SNLVRLConstructor constructor(designLibrary);
  using Paths = std::vector<std::filesystem::path>;
  Paths inputPaths;
  for (int i = 0; i < PyList_Size(arg1); ++i) {
    PyObject* object = PyList_GetItem(arg1, i);
    if (not PyUnicode_Check(object)) {
      setError("SNLDB loadSNL argument should be a file path");
      Py_RETURN_FALSE;
    }
    std::string pathStr = PyUnicode_AsUTF8(object);
    const std::filesystem::path path(pathStr);
    inputPaths.push_back(path);
  }
  constructor.construct(inputPaths);
  auto top = SNLUtils::findTop(designLibrary);
  if (top) {
    SNLUniverse::get()->setTopDesign(top);
  } else {
    setError("No top design was found after parsing verilog");
  }
  Py_RETURN_TRUE;
}

PyObject* PySNLDB_dumpVerilog(PySNLDB* self, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.dumpVerilog", &arg)) {
    setError("malformed SNLDB dumpVerilog");
    Py_RETURN_FALSE;
  }
  if (not PyUnicode_Check(arg)) {
    setError("SNLDB dumpVerilog argument should be a file path");
    Py_RETURN_FALSE;
  }
  std::ofstream output(PyUnicode_AsUTF8(arg));
  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.dumpDesign(self->object_->getTopDesign(), output);
  // return true to python
  Py_RETURN_TRUE;
}

GetObjectByName(SNLDB, SNLLibrary, getLibrary)
GetContainerMethod(DB, Library, Libraries, Libraries)

DBoDestroyAttribute(PySNLDB_destroy, PySNLDB)
            PyMethodDef PySNLDB_Methods[] = {
                {"create", (PyCFunction)PySNLDB_create,
                 METH_VARARGS | METH_STATIC, "create a SNLDB."},
                {"loadSNL", (PyCFunction)PySNLDB_loadSNL,
                 METH_VARARGS | METH_STATIC, "create a SNLDB from SNL format."},
                {"dumpSNL", (PyCFunction)PySNLDB_dumpSNL, METH_VARARGS,
                 "dump this SNLDB to SNL format."},
                {"loadLibertyPrimitives", (PyCFunction)PySNLDB_loadLibertyPrimitives,
                 METH_VARARGS, "import primitives from Liberty format."},
                {"loadVerilog", (PyCFunction)PySNLDB_loadVerilog,
                 METH_VARARGS, "create a design from Verilog format."},
                {"dumpVerilog", (PyCFunction)PySNLDB_dumpVerilog, METH_VARARGS,
                 "dump this SNLDB to SNL format."},
                {"getLibrary", (PyCFunction)PySNLDB_getLibrary, METH_VARARGS,
                 "retrieve a SNLLibrary."},
                {"getLibraries", (PyCFunction)PySNLDB_getLibraries, METH_NOARGS,
                 "get a container of SNLLibraries."},
                {"destroy", (PyCFunction)PySNLDB_destroy, METH_NOARGS,
                 "destroy this SNLDB."},
                {NULL, NULL, 0, NULL} /* sentinel */
};

DBoDeallocMethod(SNLDB)

DBoLinkCreateMethod(SNLDB) PyTypeSNLFinalObjectWithSNLIDLinkPyType(SNLDB)
PyTypeObjectDefinitions(SNLDB)
}  // namespace PYSNL
