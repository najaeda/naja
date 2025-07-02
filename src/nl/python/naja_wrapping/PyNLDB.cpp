// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLDB.h"

#include "PyInterface.h"
#include "PyNLUniverse.h"
#include "PyNLLibraries.h"
#include "PyNLLibrary.h"

#include "PySNLDesign.h"

#include "NLDB.h"

#include <Python.h>
#include <filesystem>
#include "SNLCapnP.h"
#include "SNLLibertyConstructor.h"
#include "SNLUtils.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"

#include <fstream>
#include <iostream>

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(NLDB, function)

static PyObject* PyNLDB_create(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.create", &arg)) {
    setError("malformed NLDB create");
    return nullptr;
  }
  if (not IsPyNLUniverse(arg)) {
    setError("NLDB create argument should be a NLUniverse");
    return nullptr;
  }
  auto universe = PYNLUNIVERSE_O(arg);
  NLDB* db = nullptr;
  if (universe == nullptr) {
    setError("NLDB create NLUniverse is null");
    return nullptr;
  }
  TRY
  db = NLDB::create(universe);
  NLCATCH // LCOV_EXCL_LINE should throw if universe is null, already checked
  return PyNLDB_Link(db);
}

static PyObject* PyNLDB_loadNajaIF(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.loadNajaIF", &arg)) {
    setError("malformed NLDB loadNajaIF");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    std::ostringstream oss;
    oss << "NLDB loadNajaIF argument should be a file path, got: " 
      << getStringForPyObject(arg);
    setError(oss.str());
    return nullptr;
  }
  NLDB* db = nullptr;
  const std::filesystem::path path(PyUnicode_AsUTF8(arg));
  if (NLUniverse::get() == nullptr) {
    NLUniverse::create();
  }
  TRY 
  db = SNLCapnP::load(path);
  NLUniverse::get()->setTopDesign(db->getTopDesign());
  NLCATCH
  return PyNLDB_Link(db);
}

PyObject* PyNLDB_dumpNajaIF(PyNLDB* self, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.dumpNajaIF", &arg)) {
    setError("malformed NLDB dumpNajaIF");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    std::ostringstream oss;
    oss << "NLDB dumpNajaIF argument should be a file path, got:"
      << getStringForPyObject(arg);
    setError(oss.str());
    return nullptr;
  }
  METHOD_HEAD("NLDB.dumpNajaIF()")
  SNLCapnP::dump(self->object_, PyUnicode_AsUTF8(arg));
  Py_RETURN_NONE;
}

PyObject* PyNLDB_loadLibertyPrimitives(PyNLDB* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.loadLibertyPrimitives", &arg0)) {
    setError("malformed NLDB loadLibertyPrimitives");
    return nullptr;
  }
  if (not PyList_Check(arg0)) {
    setError("malformed NLDB.loadLibertyPrimitives method");
    return nullptr;
  }
  METHOD_HEAD("NLDB.loadLibertyPrimitives()")
  NLLibrary* primitivesLibrary = nullptr;
  primitivesLibrary = selfObject->getLibrary(NLName("PRIMS"));
  if (primitivesLibrary == nullptr) {
    primitivesLibrary =
      NLLibrary::create(selfObject, NLLibrary::Type::Primitives, NLName("PRIMS"));
  }
  for (int i = 0; i < PyList_Size(arg0); ++i) {
    PyObject* object = PyList_GetItem(arg0, i);
    if (not PyUnicode_Check(object)) {
      setError("NLDB loadLibertyPrimitives argument should be a file path");
      return nullptr;
    }
    std::string pathStr = PyUnicode_AsUTF8(object);
    const std::filesystem::path path(pathStr);

    auto extension = path.extension();
    if (extension.empty()) {
      setError("NLDB loadLibertyPrimitives design path has no extension");
      return nullptr;
    } else if (extension == ".lib") {
      // LCOV_EXCL_START
      TRY
      SNLLibertyConstructor constructor(primitivesLibrary);
      constructor.construct(path);
      NLCATCH
      // LCOV_EXCL_STOP
    } else {
      setError("NLDB loadLibertyPrimitives");
      return nullptr;
    }
  }
  Py_RETURN_NONE;
}

PyObject* PyNLDB_loadVerilog(PyNLDB* self, PyObject* args, PyObject* kwargs) {
  PyObject* files = nullptr;
  int keep_assigns = 1;  // Default: true

  static const char* const kwords[] = {"files", "keep_assigns", nullptr};

  if (not PyArg_ParseTupleAndKeywords(
    args, kwargs, "O|p:NLDB.loadVerilog",
    const_cast<char**>(kwords), 
    &files, &keep_assigns)) {
    setError("malformed NLDB loadVerilog");
    return nullptr;
  }

  if (not PyList_Check(files)) {
    setError("malformed NLDB.loadVerilog method");
    return nullptr;
  }
  NLDB* db = self->object_;
  TRY
  //loadVerilog can be called multiple times
  //last one gets top
  NLLibrary* designLibrary = db->getLibrary(NLName("DESIGN"));
  if (designLibrary == nullptr) {
    designLibrary = NLLibrary::create(db, NLName("DESIGN"));
  }
  SNLVRLConstructor constructor(designLibrary);
  using Paths = std::vector<std::filesystem::path>;
  Paths inputPaths;
  for (int i = 0; i < PyList_Size(files); ++i) {
    PyObject* object = PyList_GetItem(files, i);
    if (not PyUnicode_Check(object)) {
      setError("NLDB loadVerilog argument should be a file path");
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
    NLUniverse::get()->setTopDesign(top);
    NLUniverse::get()->setTopDB(top->getDB());
  } else {
    setError("No top design was found after parsing verilog"); //LCOV_EXCL_LINE
    return nullptr; //LCOV_EXCL_LINE
  }
  NLCATCH
  Py_RETURN_NONE;
}

PyObject* PyNLDB_dumpVerilog(PyNLDB* self, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.dumpVerilog", &arg)) {
    setError("malformed NLDB dumpVerilog");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    setError("NLDB dumpVerilog argument should be a file path");
    return nullptr;
  }
  std::ofstream output(PyUnicode_AsUTF8(arg));
  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.dumpDesign(self->object_->getTopDesign(), output);
  Py_RETURN_NONE;
}

PyObject* PyNLDB_getLibrary(PyNLDB* self, PyObject* arg) {
  NLLibrary* lib = nullptr;
  METHOD_HEAD("NLDB.getLibrary()")
  if (PyUnicode_Check(arg)) {
    const char* name = PyUnicode_AsUTF8(arg);
    lib = selfObject->getLibrary(NLName(name));
  } else if (PyLong_Check(arg)) {
    int index = PyLong_AsLong(arg);
    lib = selfObject->getLibrary(index);
  } else {
      setError("invalid number of parameters for getLibrary.");
      return nullptr;
  }
  return PyNLLibrary_Link(lib);
}

DirectGetIntMethod(PyNLDB_getID, getID, PyNLDB, NLDB)

GetBoolAttribute(NLDB, isTopDB)
GetObjectMethod(NLDB, SNLDesign, getTopDesign)

GetContainerMethod(NLDB, NLLibrary*, NLLibraries, Libraries)
GetContainerMethod(NLDB, NLLibrary*, NLLibraries, GlobalLibraries)
GetContainerMethod(NLDB, NLLibrary*, NLLibraries, PrimitiveLibraries)

DBoDestroyAttribute(PyNLDB_destroy, PyNLDB)

PyMethodDef PyNLDB_Methods[] = {
  { "create", (PyCFunction)PyNLDB_create, METH_VARARGS | METH_STATIC,
    "create a NLDB."},
  { "getID", (PyCFunction)PyNLDB_getID, METH_NOARGS,
    "get the NLDB ID."},
  { "isTopDB", (PyCFunction)PyNLDB_isTopDB, METH_NOARGS,
    "Returns True if the NLDB is the top DB."},
  { "getTopDesign", (PyCFunction)PyNLDB_getTopDesign, METH_NOARGS,
    "get the top design."},
  { "loadNajaIF", (PyCFunction)PyNLDB_loadNajaIF, METH_VARARGS | METH_STATIC,
    "create a NLDB from Naja Interchange format."},
  { "dumpNajaIF", (PyCFunction)PyNLDB_dumpNajaIF, METH_VARARGS,
    "dump this NLDB to SNL format."},
  { "loadLibertyPrimitives", (PyCFunction)PyNLDB_loadLibertyPrimitives, METH_VARARGS,
    "import primitives from Liberty format."},
  { "loadVerilog", (PyCFunction)PyNLDB_loadVerilog, METH_VARARGS|METH_KEYWORDS,
    "create a design from Verilog format."},
  { "dumpVerilog", (PyCFunction)PyNLDB_dumpVerilog, METH_VARARGS,
    "dump this NLDB to SNL format."},
  { "getLibrary", (PyCFunction)PyNLDB_getLibrary, METH_O,
    "retrieve a NLLibrary."},
  { "getLibraries", (PyCFunction)PyNLDB_getLibraries, METH_NOARGS,
    "iterate on this NLDB SNLLibraries."},
  { "getGlobalLibraries", (PyCFunction)PyNLDB_getGlobalLibraries, METH_NOARGS,
    "iterate on all the Libraries owned (directly or indirectly) by this NLDB."},
  { "getPrimitiveLibraries", (PyCFunction)PyNLDB_getPrimitiveLibraries, METH_NOARGS,
    "iterate on all the primitive Libraries owned (directly or indirectly) by this NLDB."},
  { "destroy", (PyCFunction)PyNLDB_destroy, METH_NOARGS,
    "destroy this NLDB."},
  {NULL, NULL, 0, NULL} /* sentinel */
};

DBoDeallocMethod(NLDB)

DBoLinkCreateMethod(NLDB)
PyTypeNLFinalObjectWithNLIDLinkPyType(NLDB)
PyTypeObjectDefinitions(NLDB)

}  // namespace PYNAJA
