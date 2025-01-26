// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLLibrary.h"

#include "PyInterface.h"
#include "PySNLDB.h"
#include "PySNLDesign.h"
#include "PySNLDesigns.h"

#include "SNLLibrary.h"

using namespace naja::SNL;

namespace PYSNL {

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLLibrary, function)

namespace {

static PyObject* createLibrary(PyObject* args, SNLLibrary::Type type) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:SNLLibrary.createLibrary", &arg0, &arg1)) {
    setError("malformed SNLLibrary create");
    return nullptr;
  }
  SNLName name;
  if (arg1) {
    name = SNLName(arg1);
  }

  SNLLibrary* lib = nullptr;
  TRY
  if (IsPySNLDB(arg0)) {
    lib = SNLLibrary::create(PYSNLDB_O(arg0), type, name);
  } else if (IsPySNLLibrary(arg0)) {
    lib = SNLLibrary::create(PYSNLLibrary_O(arg0), type, name);
  } else {
    setError("SNLLibrary creator accepts as first argument either a SNLDB or a SNLLibrary");
    return nullptr;
  }
  SNLCATCH
  return PySNLLibrary_Link(lib);
}

}

static PyObject* PySNLLibrary_create(PyObject*, PyObject* args) {
  return createLibrary(args, SNLLibrary::Type::Standard);
}

static PyObject* PySNLLibrary_createPrimitives(PyObject*, PyObject* args) {
  return createLibrary(args, SNLLibrary::Type::Primitives);
}

static PyObject* PySNLLibrary_getDesign(PySNLLibrary* self, PyObject* arg) {
  SNLDesign* design = nullptr;
  METHOD_HEAD("SNLLibrary.getDesign()")
  if (PyUnicode_Check(arg)) {
    const char* name = PyUnicode_AsUTF8(arg);
    design = selfObject->getDesign(SNLName(name));
  } else if (PyLong_Check(arg)) {
    int index = PyLong_AsLong(arg);
    design = selfObject->getDesign(index);
  } else {
      setError("invalid number of parameters for getDesign.");
      return nullptr;
  }
  return PySNLDesign_Link(design);
}

GetObjectMethod(Library, DB, getDB)
GetObjectByName(SNLLibrary, SNLLibrary, getLibrary)

DirectGetIntMethod(PySNLLibrary_getID, getID, PySNLLibrary, SNLLibrary)

GetBoolAttribute(Library, isStandard)
GetBoolAttribute(Library, isPrimitives)

SetNameMethod(Library)
GetNameMethod(SNLLibrary)

GetContainerMethod(Library, Design*, Designs, Designs)

DBoDeallocMethod(SNLLibrary)

DBoLinkCreateMethod(SNLLibrary)
PyTypeObjectDefinitions(SNLLibrary)

PyMethodDef PySNLLibrary_Methods[] = {
  { "create", (PyCFunction)PySNLLibrary_create, METH_VARARGS|METH_STATIC,
    "SNLLibrary creator"},
  {"setName", (PyCFunction)PySNLLibrary_setName, METH_O,
    "Set the SNLName of this SNLLibrary."},
  { "createPrimitives", (PyCFunction)PySNLLibrary_createPrimitives, METH_VARARGS|METH_STATIC,
    "Primitives SNLLibrary creator"},
  { "getName", (PyCFunction)PySNLLibrary_getName, METH_NOARGS,
    "get SNLLibrary name"},
  { "isStandard", (PyCFunction)PySNLLibrary_isStandard, METH_NOARGS,
    "is this a standard SNLLibrary?"},
  { "isPrimitives", (PyCFunction)PySNLLibrary_isPrimitives, METH_NOARGS,
    "is this a primitives SNLLibrary?"},
  { "getID", (PyCFunction)PySNLLibrary_getID, METH_NOARGS,
    "get the ID."},
  { "getDB", (PyCFunction)PySNLLibrary_getDB, METH_VARARGS,
    "get Parent DB."},
  { "getLibrary", (PyCFunction)PySNLLibrary_getLibrary, METH_VARARGS,
    "retrieve a SNLLibrary."},
  { "getDesign", (PyCFunction)PySNLLibrary_getDesign, METH_O,
    "retrieve a SNLDesign."},
  { "getSNLDesigns", (PyCFunction)PySNLLibrary_getSNLDesigns, METH_NOARGS,
    "get a container of SNLDesigns."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeSNLFinalObjectWithSNLIDLinkPyType(SNLLibrary)

}
