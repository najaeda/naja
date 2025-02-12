// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBusTerm.h"

#include "PyInterface.h"
#include "PySNLDesign.h"
#include "PySNLBusTermBit.h"
#include <limits>

#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)   GENERIC_METHOD_HEAD(SNLBusTerm, function)

static PyObject* PySNLBusTerm_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  int arg1 = 0;
  int arg2 = 0;
  int arg3 = 0;
  const char* arg4 = nullptr;
  if (not PyArg_ParseTuple(args, "Oiii|s:SNLDB.create", &arg0, &arg1, &arg2, &arg3, &arg4)) {
    setError("malformed SNLBusTerm create method");
    return nullptr;
  }
  SNLTerm::Direction direction = SNLNetComponent::Direction::DirectionEnum(arg1);
  SNLName name;
  if (arg4) {
    name = SNLName(arg4);
  }

  SNLBusTerm* term = nullptr;
  TRY
  if (IsPySNLDesign(arg0)) {
    term = SNLBusTerm::create(PYSNLDesign_O(arg0), direction, arg2, arg3, name);
  } else {
    setError("SNLBusTerm create accepts SNLDesign as first argument");
    return nullptr;
  }
  SNLCATCH
  return PySNLBusTerm_Link(term);
}

// To retrun getBit but to rename the function as getBusTermBit
PyObject* PySNLBusTerm_getBusTermBit(PySNLBusTerm* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLBusTerm.getBit", &arg0)) {
    setError("malformed SNLBusTerm getBit method");
    return nullptr;
  }
  SNLID::Bit bit = 0;
  if (PyLong_Check(arg0)) {
    bit = PyLong_AsLong(arg0);
  } else {
    setError("SNLBusTerm getBit accepts an integer as first argument");
    return nullptr;
  }
  SNLBusTermBit* bitTerm = nullptr;
  // LCOV_EXCL_START
  TRY
  bitTerm = (static_cast<naja::SNL::SNLBusTerm*>(self->parent_.parent_.parent_.object_))->getBit(bit);
  SNLCATCH
  // LCOV_EXCL_STOP
  return PySNLBusTermBit_Link(bitTerm);
}

// Function named getBit with no args that returns -1 int
PyObject* PySNLBusTerm_getBit(PySNLBusTerm* self) {
  return PyLong_FromLong(std::numeric_limits<long>::max());
}

DirectGetIntMethod(PySNLBusTerm_getMSB, getMSB, PySNLBusTerm, SNLBusTerm)
DirectGetIntMethod(PySNLBusTerm_getLSB, getLSB, PySNLBusTerm, SNLBusTerm)
DirectGetIntMethod(PySNLBusTerm_getID, getID, PySNLBusTerm, SNLBusTerm)
DirectGetIntMethod(PySNLBusTerm_getFlatID, getFlatID, PySNLBusTerm, SNLBusTerm)

//GetObjectByIndex(BusTerm, BusTermBit, Bit)

DBoLinkCreateMethod(SNLBusTerm)
DBoDeallocMethod(SNLBusTerm)
PyTypeObjectDefinitions(SNLBusTerm)

PyMethodDef PySNLBusTerm_Methods[] = {
  { "create", (PyCFunction)PySNLBusTerm_create, METH_VARARGS|METH_STATIC,
    "SNLBusTerm creator"},
  { "getMSB", (PyCFunction)PySNLBusTerm_getMSB, METH_NOARGS,
    "get SNLBusTerm MSB value"},
  { "getLSB", (PyCFunction)PySNLBusTerm_getLSB, METH_NOARGS,
    "get SNLBusTerm LSB value"},
  { "getBit", (PyCFunction)PySNLBusTerm_getBit, METH_NOARGS,
    "retruns -1"},
  { "getBusTermBit", (PyCFunction)PySNLBusTerm_getBusTermBit, METH_VARARGS,
    "get SNLBusTerm Bit, returns SNLBusTermBit"},
  { "getID", (PyCFunction)PySNLBusTerm_getID, METH_NOARGS,
    "get SNLBusTerm ID value"},
  { "getFlatID", (PyCFunction)PySNLBusTerm_getFlatID, METH_NOARGS,
    "get SNLBusTerm FlatID value"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeSNLFinalObjectWithSNLIDLinkPyType(SNLBusTerm)

}
