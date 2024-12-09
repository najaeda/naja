// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLUniverse.h"

#include "PyInterface.h"
#include "PySNLDesign.h"
#include "PySNLDB.h"
#include "PySNLDBs.h"

#include "SNLUniverse.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLUniverse, function)

static PyObject* PySNLUniverse_create() {
  SNLUniverse* universe = nullptr;
  TRY
  universe = SNLUniverse::create();
  SNLCATCH
  return PySNLUniverse_Link(universe);
}

static PyObject* PySNLUniverse_get() {
  auto universe = SNLUniverse::get();
  return PySNLUniverse_Link(universe);
}

static PyObject* PySNLUniverse_setTopDesign(PySNLUniverse* self, PyObject* arg) {
  METHOD_HEAD("SNLUniverse.setTopDesign()")
  if (IsPySNLDesign(arg)) {
    selfObject->setTopDesign(PYSNLDesign_O(arg));
  } else {
    setError("SNLUniverse setTopDesign takes SNLDesign argument");
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* PySNLUniverse_setTopDB(PySNLUniverse* self, PyObject* arg) {
  METHOD_HEAD("SNLUniverse.setTopDB()")
  if (IsPySNLDB(arg)) {
    selfObject->setTopDB(PYSNLDB_O(arg));
  } else {
    setError("SNLUniverse setTopDB takes SNLDesign argument");
    return nullptr;
  }
  Py_RETURN_NONE;
}

GetObjectMethod(Universe, Design, getTopDesign)
GetObjectMethod(Universe, DB, getTopDB)
GetObjectByIndex(Universe, DB, DB)
GetContainerMethod(Universe, DB, DBs, UserDBs)

DBoDestroyAttribute(PySNLUniverse_destroy, PySNLUniverse)

PyMethodDef PySNLUniverse_Methods[] = {
  { "create", (PyCFunction)PySNLUniverse_create, METH_NOARGS|METH_STATIC,
    "create the SNL Universe (static object)"},
  { "destroy", (PyCFunction)PySNLUniverse_destroy, METH_NOARGS,
    "destroy the associated SNLUniverse"},
  { "get", (PyCFunction)PySNLUniverse_get, METH_NOARGS|METH_STATIC,
    "get the SNL Universe (static object)"},
  { "getTopDesign", (PyCFunction)PySNLUniverse_getTopDesign, METH_NOARGS,
    "get the top SNLDesign"},
  { "setTopDesign", (PyCFunction)PySNLUniverse_setTopDesign, METH_O,
    "set the top SNLDesign"},
  { "setTopDB", (PyCFunction)PySNLUniverse_setTopDB, METH_O,
    "set the top SNLDB"},
  { "getTopDB", (PyCFunction)PySNLUniverse_getTopDB, METH_NOARGS,
    "get the Top SNLDB"},
  { "getDB", (PyCFunction)PySNLUniverse_getDB, METH_VARARGS,
    "get the SNLDB with the given index"},
  { "getUserDBs", (PyCFunction)PySNLUniverse_getUserDBs, METH_NOARGS,
    "iterate on User SNLDBs."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLUniverse)

DBoLinkCreateMethod(SNLUniverse)
PyTypeSNLObjectWithoutSNLIDLinkPyType(SNLUniverse)
PyTypeObjectDefinitions(SNLUniverse)

}