// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLUniverse.h"

#include "PyInterface.h"
#include "PySNLDesign.h"
#include "PySNLDB.h"

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

GetObjectMethod(Universe, Design, getTopDesign)
GetObjectMethod(Universe, DB, getTopDB)
GetObjectByIndex(Universe, DB, DB)

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
  { "getTopDB", (PyCFunction)PySNLUniverse_getTopDB, METH_NOARGS,
    "get the Top SNLDB"},
  { "getDB", (PyCFunction)PySNLUniverse_getDB, METH_VARARGS,
    "get the SNLDB with the given index"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLUniverse)

DBoLinkCreateMethod(SNLUniverse)
PyTypeSNLObjectWithoutSNLIDLinkPyType(SNLUniverse)
PyTypeObjectDefinitions(SNLUniverse)

}