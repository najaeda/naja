// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
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
  if (not PyArg_ParseTuple(args, "O|s:SNLLibrary.createPrimitives", &arg0, &arg1)) {
    setError("malformed Primitives SNLLibrary create");
    return nullptr;
  }
  SNLName name;
  if (arg1) {
    name = SNLName(arg1);
  }

  SNLLibrary* lib = nullptr;
  SNLTRY
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

GetObjectMethod(Library, DB)
GetObjectByName(Library, Design)
GetObjectByName(Library, Library)

GetNameMethod(SNLLibrary)

GetContainerMethod(Library, Design)

DBoDeallocMethod(SNLLibrary)

DBoLinkCreateMethod(SNLLibrary)
PyTypeObjectDefinitions(SNLLibrary)

PyMethodDef PySNLLibrary_Methods[] = {
  { "create", (PyCFunction)PySNLLibrary_create, METH_VARARGS|METH_STATIC,
    "SNLLibrary creator"},
  { "createPrimitives", (PyCFunction)PySNLLibrary_createPrimitives, METH_VARARGS|METH_STATIC,
    "Primitives SNLLibrary creator"},
  { "getName", (PyCFunction)PySNLLibrary_getName, METH_NOARGS,
    "get SNLLibrary name"},
  { "getDB", (PyCFunction)PySNLLibrary_getDB, METH_VARARGS,
    "get Parent DB."},
  { "getLibrary", (PyCFunction)PySNLLibrary_getLibrary, METH_VARARGS,
    "retrieve a SNLLibrary."},
  { "getDesign", (PyCFunction)PySNLLibrary_getDesign, METH_VARARGS,
    "retrieve a SNLDesign."},
  { "getDesigns", (PyCFunction)PySNLLibrary_getDesigns, METH_NOARGS,
    "get a container of SNLDesigns."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeSNLObjectWithSNLIDLinkPyType(SNLLibrary)

}
