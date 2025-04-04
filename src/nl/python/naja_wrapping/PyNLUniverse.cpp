// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLUniverse.h"

#include "PyInterface.h"
#include "PyNLDB.h"
#include "PyNLDBs.h"
#include "PySNLDesign.h"

#include "NLUniverse.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(NLUniverse, function)

static PyObject* PyNLUniverse_create() {
  NLUniverse* universe = nullptr;
  TRY
  universe = NLUniverse::create();
  NLCATCH
  return PyNLUniverse_Link(universe);
}

static PyObject* PyNLUniverse_get() {
  auto universe = NLUniverse::get();
  return PyNLUniverse_Link(universe);
}

static PyObject* PyNLUniverse_setTopDesign(PyNLUniverse* self, PyObject* arg) {
  METHOD_HEAD("NLUniverse.setTopDesign()")
  if (IsPySNLDesign(arg)) {
    selfObject->setTopDesign(PYSNLDesign_O(arg));
  } else {
    setError("NLUniverse setTopDesign takes SNLDesign argument");
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* PyNLUniverse_setTopDB(PyNLUniverse* self, PyObject* arg) {
  METHOD_HEAD("NLUniverse.setTopDB()")
  if (IsPyNLDB(arg)) {
    selfObject->setTopDB(PYNLDB_O(arg));
  } else {
    setError("NLUniverse setTopDB takes SNLDesign argument");
    return nullptr;
  }
  Py_RETURN_NONE;
}

GetObjectMethod(NLUniverse, SNLDesign, getTopDesign)
GetObjectMethod(NLUniverse, NLDB, getTopDB)
GetObjectByIndex(NLUniverse, NLDB, DB)
GetContainerMethod(NLUniverse, NLDB*, NLDBs, UserDBs)

DBoDestroyAttribute(PyNLUniverse_destroy, PyNLUniverse)

PyMethodDef PyNLUniverse_Methods[] = {
  { "create", (PyCFunction)PyNLUniverse_create, METH_NOARGS|METH_STATIC,
    "create the NLUniverse (static object)"},
  { "destroy", (PyCFunction)PyNLUniverse_destroy, METH_NOARGS,
    "destroy the associated NLUniverse"},
  { "get", (PyCFunction)PyNLUniverse_get, METH_NOARGS|METH_STATIC,
    "get the NLUniverse (static object)"},
  { "getTopDesign", (PyCFunction)PyNLUniverse_getTopDesign, METH_NOARGS,
    "get the top SNLDesign"},
  { "setTopDesign", (PyCFunction)PyNLUniverse_setTopDesign, METH_O,
    "set the top SNLDesign"},
  { "setTopDB", (PyCFunction)PyNLUniverse_setTopDB, METH_O,
    "set the top NLDB"},
  { "getTopDB", (PyCFunction)PyNLUniverse_getTopDB, METH_NOARGS,
    "get the Top NLDB"},
  { "getDB", (PyCFunction)PyNLUniverse_getDB, METH_VARARGS,
    "get the NLDB with the given index"},
  { "getUserDBs", (PyCFunction)PyNLUniverse_getUserDBs, METH_NOARGS,
    "iterate on User NLDBs."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(NLUniverse)

DBoLinkCreateMethod(NLUniverse)
PyTypeNLObjectWithoutNLIDLinkPyType(NLUniverse)
PyTypeObjectDefinitions(NLUniverse)

}