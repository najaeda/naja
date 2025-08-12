// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLUniverse.h"

#include "PyInterface.h"
#include "PyNLDB.h"
#include "PyNLDBs.h"
#include "PySNLDesign.h"

#include "NLUniverse.h"
#include "RemoveLoadlessLogic.h"
#include "ConstantPropagation.h"
#include "FanoutComputer.h"
#include "LogicLevelComputer.h"

namespace PYNAJA {

using namespace naja::NL;
using namespace naja::NAJA_OPT;
using namespace naja::NAJA_METRICS;


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

static PyObject* PyNLUniverse_applyDLE() {
 LoadlessLogicRemover remover;
 remover.setNormalizedUniquification(true);
 remover.process();
 Py_RETURN_NONE;
}

// fanout calculation
static PyObject* PyNLUniverse_getMaxFanout() {
  FanoutComputer fanoutComputer;
  fanoutComputer.process();
  return PyLong_FromSize_t(fanoutComputer.getMaxFanout());
}

static PyObject* PyNLUniverse_getMaxLogicLevel() {
  LogicLevelComputer logicLevelComputer;
  logicLevelComputer.process();
  return PyLong_FromSize_t(logicLevelComputer.getMaxLogicLevel());
}

static PyObject* PyNLUniverse_applyConstantPropagation() {
  ConstantPropagation cp;
  cp.setTruthTableEngine(true);
  cp.run();
  Py_RETURN_NONE;
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
  { "applyDLE", (PyCFunction)PyNLUniverse_applyDLE, METH_NOARGS|METH_STATIC,
   "apply Dead Logic Elimination on the top design of the NLUniverse."},
  { "applyConstantPropagation", (PyCFunction)PyNLUniverse_applyConstantPropagation, METH_NOARGS|METH_STATIC,
    "apply Constant Propagation on the top design of the NLUniverse."},
  { "getMaxFanout", (PyCFunction)PyNLUniverse_getMaxFanout, METH_NOARGS|METH_STATIC,
    "get the maximum fanout of the top design of the NLUniverse."},
  { "getMaxLogicLevel", (PyCFunction)PyNLUniverse_getMaxLogicLevel, METH_NOARGS|METH_STATIC,
    "get the maximum logic level of the top design of the NLUniverse."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(NLUniverse)

DBoLinkCreateMethod(NLUniverse)
PyTypeNLObjectWithoutNLIDLinkPyType(NLUniverse)
PyTypeObjectDefinitions(NLUniverse)

}