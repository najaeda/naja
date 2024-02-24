// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLNetComponent.h"

#include "PyInterface.h"
#include "PySNLBitNet.h"

namespace PYSNL {

using namespace naja::SNL;

#undef ACCESS_OBJECT
#undef ACCESS_CLASS
#define ACCESS_OBJECT           parent_.object_
#define ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLNetComponent, function)

static PyObject* PySNLNetComponent_getNet(PySNLNetComponent* self) {
  METHOD_HEAD("SNLNetComponent.getNet()")
  SNLBitNet* net = selfObject->getNet();
  return PySNLBitNet_Link(net);
}

static PyObject* PySNLNetComponent_setNet(PySNLNetComponent* self, PyObject* arg) {
  METHOD_HEAD("SNLNetComponent.setNet()")
  if (IsPySNLNet(arg)) {
    SNLTRY
    selfObject->setNet(PYSNLNet_O(arg));
    SNLCATCH
  } else {
    setError("SNLNetComponent setNet takes SNLNet argument");
    return nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject* PySNLNetComponent_getDirection(PySNLNetComponent* self) {
  METHOD_HEAD("Net.getDirection()")
  return PyLong_FromLong((long)selfObject->getDirection());
}

PyMethodDef PySNLNetComponent_Methods[] = {
  { "getNet", (PyCFunction)PySNLNetComponent_getNet, METH_NOARGS,
    "get SNLNetComponent SNLBitNet"},
  { "setNet", (PyCFunction)PySNLNetComponent_setNet, METH_O,
    "set SNLNetComponent SNLNet"},
  { "getDirection", (PyCFunction)PySNLNetComponent_getDirection, METH_NOARGS,
    "get SNLNetComponent direction"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLNetComponent)

DBoLinkCreateMethod(SNLNetComponent)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLNetComponent)
PyTypeInheritedObjectDefinitions(SNLNetComponent, SNLDesignObject)

}