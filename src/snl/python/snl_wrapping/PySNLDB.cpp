// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDB.h"

#include "PyInterface.h"
#include "PySNLUniverse.h"
#include "PySNLLibraries.h"
#include "PySNLLibrary.h"
#include "PySNLDesign.h"

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
  if (universe == nullptr) {
    setError("SNLDB create SNLUniverse is null");
    return nullptr;
  }
  TRY
  db = SNLDB::create(universe);
  SNLCATCH // LCOV_EXCL_LINE should throw if universe is null, already checked
  return PySNLDB_Link(db);
}

static PyObject* PySNLDB_loadSNL(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.loadSNL", &arg)) {
    setError("malformed SNLDB loadSNL");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    std::ostringstream oss;
    oss << "SNLDB loadSNL argument should be a file path, got: " 
      << getStringForPyObject(arg);
    setError(oss.str());
    return nullptr;
  }
  SNLDB* db = nullptr;
  const std::filesystem::path path(PyUnicode_AsUTF8(arg));
  if (SNLUniverse::get() == nullptr) {
    SNLUniverse::create();
  }
  TRY 
  db = SNLCapnP::load(path);
  SNLUniverse::get()->setTopDesign(db->getTopDesign());
  SNLCATCH
  return PySNLDB_Link(db);
}

PyObject* PySNLDB_dumpSNL(PySNLDB* self, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.dumpSNL", &arg)) {
    setError("malformed SNLDB dumpSNL");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    std::ostringstream oss;
    oss << "SNLDB dumpSNL argument should be a file path, got:"
      << getStringForPyObject(arg);
    setError(oss.str());
    return nullptr;
  }
  METHOD_HEAD("SNLDesign.setTruthTable()")
  SNLCapnP::dump(self->object_, PyUnicode_AsUTF8(arg));
  Py_RETURN_NONE;
}

PyObject* PySNLDB_loadLibertyPrimitives(PySNLDB* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.loadLibertyPrimitives", &arg0)) {
    setError("malformed SNLDB loadLibertyPrimitives");
    return nullptr;
  }
  if (not PyList_Check(arg0)) {
    setError("malformed SNLDesign.loadLibertyPrimitives method");
    return nullptr;
  }
  METHOD_HEAD("SNLDB.loadLibertyPrimitives()")
  SNLLibrary* primitivesLibrary = nullptr;
  primitivesLibrary = selfObject->getLibrary(SNLName("PRIMS"));
  if (primitivesLibrary == nullptr) {
    primitivesLibrary =
      SNLLibrary::create(selfObject, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  }
  for (int i = 0; i < PyList_Size(arg0); ++i) {
    PyObject* object = PyList_GetItem(arg0, i);
    if (not PyUnicode_Check(object)) {
      setError("SNLDB loadLibertyPrimitives argument should be a file path");
      return nullptr;
    }
    std::string pathStr = PyUnicode_AsUTF8(object);
    const std::filesystem::path path(pathStr);

    auto extension = path.extension();
    if (extension.empty()) {
      setError("SNLDB loadLibertyPrimitives design path has no extension");
      return nullptr;
    } else if (extension == ".lib") {
      // LCOV_EXCL_START
      TRY
      SNLLibertyConstructor constructor(primitivesLibrary);
      constructor.construct(path);
      SNLCATCH
      // LCOV_EXCL_STOP
    } else {
      setError("SNLDB loadLibertyPrimitives");
      return nullptr;
    }
  }
  Py_RETURN_NONE;
}

PyObject* PySNLDB_loadVerilog(PySNLDB* self, PyObject* args, PyObject* kwargs) {
  PyObject* files = nullptr;
  int keep_assigns = 1;  // Default: true

  static const char* const kwords[] = {"files", "keep_assigns", nullptr};

  if (not PyArg_ParseTupleAndKeywords(
    args, kwargs, "O|p:SNLDB.loadVerilog",
    const_cast<char**>(kwords), 
    &files, &keep_assigns)) {
    setError("malformed SNLDB loadVerilog");
    return nullptr;
  }

  if (not PyList_Check(files)) {
    setError("malformed SNLDB.loadVerilog method");
    return nullptr;
  }
  SNLDB* db = self->object_;
  TRY
  //loadVerilog can be called multiple times
  //last one gets top
  SNLLibrary* designLibrary = db->getLibrary(SNLName("DESIGN"));
  if (designLibrary == nullptr) {
    designLibrary = SNLLibrary::create(db, SNLName("DESIGN"));
  }
  SNLVRLConstructor constructor(designLibrary);
  using Paths = std::vector<std::filesystem::path>;
  Paths inputPaths;
  for (int i = 0; i < PyList_Size(files); ++i) {
    PyObject* object = PyList_GetItem(files, i);
    if (not PyUnicode_Check(object)) {
      setError("SNLDB loadVerilog argument should be a file path");
      return nullptr;
    }
    std::string pathStr = PyUnicode_AsUTF8(object);
    const std::filesystem::path path(pathStr);
    inputPaths.push_back(path);
  }
  constructor.construct(inputPaths);
  if (not keep_assigns) {
    db->mergeAssigns();
  }
  auto top = SNLUtils::findTop(designLibrary);
  if (top) {
    SNLUniverse::get()->setTopDesign(top);
    SNLUniverse::get()->setTopDB(top->getDB());
  } else {
    setError("No top design was found after parsing verilog"); //LCOV_EXCL_LINE
    return nullptr; //LCOV_EXCL_LINE
  }
  SNLCATCH
  Py_RETURN_NONE;
}

PyObject* PySNLDB_dumpVerilog(PySNLDB* self, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.dumpVerilog", &arg)) {
    setError("malformed SNLDB dumpVerilog");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    setError("SNLDB dumpVerilog argument should be a file path");
    return nullptr;
  }
  std::ofstream output(PyUnicode_AsUTF8(arg));
  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.dumpDesign(self->object_->getTopDesign(), output);
  Py_RETURN_NONE;
}

PyObject* PySNLDB_getLibrary(PySNLDB* self, PyObject* arg) {
  SNLLibrary* lib = nullptr;
  METHOD_HEAD("SNLDB.getLibrary()")
  if (PyUnicode_Check(arg)) {
    const char* name = PyUnicode_AsUTF8(arg);
    lib = selfObject->getLibrary(SNLName(name));
  } else if (PyLong_Check(arg)) {
    int index = PyLong_AsLong(arg);
    lib = selfObject->getLibrary(index);
  } else {
      setError("invalid number of parameters for getLibrary.");
      return nullptr;
  }
  return PySNLLibrary_Link(lib);
}

DirectGetIntMethod(PySNLDB_getID, getID, PySNLDB, SNLDB)

GetBoolAttribute(DB, isTopDB)
GetObjectMethod(DB, Design, getTopDesign)

GetContainerMethod(DB, Library*, Libraries, Libraries)
GetContainerMethod(DB, Library*, Libraries, GlobalLibraries)
GetContainerMethod(DB, Library*, Libraries, PrimitiveLibraries)

DBoDestroyAttribute(PySNLDB_destroy, PySNLDB)

PyMethodDef PySNLDB_Methods[] = {
  { "create", (PyCFunction)PySNLDB_create, METH_VARARGS | METH_STATIC,
    "create a SNLDB."},
  { "getID", (PyCFunction)PySNLDB_getID, METH_NOARGS,
    "get the SNLDB ID."},
  { "isTopDB", (PyCFunction)PySNLDB_isTopDB, METH_NOARGS,
    "Returns True if the SNLDB is the top DB."},
  { "getTopDesign", (PyCFunction)PySNLDB_getTopDesign, METH_NOARGS,
    "get the top design."},
  { "loadSNL", (PyCFunction)PySNLDB_loadSNL, METH_VARARGS | METH_STATIC,
    "create a SNLDB from SNL format."},
  { "dumpSNL", (PyCFunction)PySNLDB_dumpSNL, METH_VARARGS,
    "dump this SNLDB to SNL format."},
  { "loadLibertyPrimitives", (PyCFunction)PySNLDB_loadLibertyPrimitives, METH_VARARGS,
    "import primitives from Liberty format."},
  { "loadVerilog", (PyCFunction)PySNLDB_loadVerilog, METH_VARARGS|METH_KEYWORDS,
    "create a design from Verilog format."},
  { "dumpVerilog", (PyCFunction)PySNLDB_dumpVerilog, METH_VARARGS,
    "dump this SNLDB to SNL format."},
  { "getLibrary", (PyCFunction)PySNLDB_getLibrary, METH_O,
    "retrieve a SNLLibrary."},
  { "getLibraries", (PyCFunction)PySNLDB_getLibraries, METH_NOARGS,
    "iterate on this SNLDB SNLLibraries."},
  { "getGlobalLibraries", (PyCFunction)PySNLDB_getGlobalLibraries, METH_NOARGS,
    "iterate on all the Libraries owned (directly or indirectly) by this SNLDB."},
  { "getPrimitiveLibraries", (PyCFunction)PySNLDB_getPrimitiveLibraries, METH_NOARGS,
    "iterate on all the primitive Libraries owned (directly or indirectly) by this SNLDB."},
  { "destroy", (PyCFunction)PySNLDB_destroy, METH_NOARGS,
    "destroy this SNLDB."},
  {NULL, NULL, 0, NULL} /* sentinel */
};

DBoDeallocMethod(SNLDB)

DBoLinkCreateMethod(SNLDB) PyTypeSNLFinalObjectWithSNLIDLinkPyType(SNLDB)
PyTypeObjectDefinitions(SNLDB)

}  // namespace PYSNL