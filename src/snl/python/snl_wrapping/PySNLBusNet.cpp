// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBusNet.h"

#include "PyInterface.h"
#include "PySNLDesign.h"
#include "PySNLBusNetBit.h"

#include "SNLBusNet.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)   GENERIC_METHOD_HEAD(SNLBusNet, function)

static PyObject* PySNLBusNet_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  int arg1 = 0;
  int arg2 = 0;
  const char* arg3 = nullptr;
  if (not PyArg_ParseTuple(args, "Oii|s:SNLDB.create", &arg0, &arg1, &arg2, &arg3)) {
    setError("malformed SNLBusNet create method");
    return nullptr;
  }
  SNLName name;
  if (arg3) {
    name = SNLName(arg3);
  }

  SNLBusNet* net = nullptr;
  SNLTRY
  if (IsPySNLDesign(arg0)) {
    net = SNLBusNet::create(PYSNLDesign_O(arg0), arg1, arg2, name);
  } else {
    setError("SNLBusNet create accepts SNLDesign as first argument");
    return nullptr;
  }
  SNLCATCH
  return PySNLBusNet_Link(net);
}

DirectGetIntMethod(PySNLBusNet_getMSB, getMSB, PySNLBusNet, SNLBusNet)
DirectGetIntMethod(PySNLBusNet_getLSB, getLSB, PySNLBusNet, SNLBusNet)
DirectGetIntMethod(PySNLBusNet_getSize, getSize, PySNLBusNet, SNLBusNet)

GetObjectByIndex(BusNet, BusNetBit, Bit)

DBoLinkCreateMethod(SNLBusNet)
DBoDeallocMethod(SNLBusNet)

PyTypeObjectDefinitions(SNLBusNet)

PyMethodDef PySNLBusNet_Methods[] = {
  { "create", (PyCFunction)PySNLBusNet_create, METH_VARARGS|METH_STATIC,
    "SNLBusNet creator"},
  { "getMSB", (PyCFunction)PySNLBusNet_getMSB, METH_NOARGS,
    "get SNLBusNet MSB value"},
  { "getLSB", (PyCFunction)PySNLBusNet_getLSB, METH_NOARGS,
    "get SNLBusNet LSB value"},
  { "getSize", (PyCFunction)PySNLBusNet_getSize, METH_NOARGS,
    "get SNLBusNet Size"},
  { "getBit", (PyCFunction)PySNLBusNet_getBit, METH_VARARGS,
    "get SNLBusNet Bit, returns SNLBusNetBit"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeSNLObjectWithSNLIDLinkPyType(SNLBusNet)

}
