// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLNet.h"

#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLDesignModeling.h"

#include "PyInterface.h"
#include "PySNLScalarNet.h"
#include "PySNLBusNet.h"
#include "PySNLDesign.h"
#include "PySNLBitNets.h"

namespace PYNAJA {

using namespace naja::NL;

#undef ACCESS_OBJECT
#undef ACCESS_CLASS
#define ACCESS_OBJECT            parent_.object_
#define ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLNet, function)

GetNameMethod(SNLNet)
GetContainerMethodWithMethodName(Net, BitNet, getBits)

PyObject* PySNLNet_Link(SNLNet* object) {
  if (not object) {
    Py_RETURN_NONE;   
  }
  if (auto busNet = dynamic_cast<SNLBusNet*>(object)) {
    return PySNLBusNet_Link(busNet);
  } else {
    auto scalarNet = static_cast<SNLScalarNet*>(object);
    return PySNLScalarNet_Link(scalarNet);
  }
}

PyTypeInheritedObjectDefinitions(SNLNet, SNLDesignObject)

DirectGetNumericMethod(PySNLNet_getWidth, getWidth, PySNLNet, SNLNet)
static bool isAllConstant(const SNLNet* net, NLLogicValue value) {
  bool hasBits = false;
  for (auto* bit: net->getBits()) {
    hasBits = true;
    if (SNLDesignModeling::getConstantValue(bit) != value) return false;
  }
  return hasBits;
}

static PyObject* PySNLNet_isConstant0(PySNLNet* self) {
  METHOD_HEAD("SNLNet.isConstant0()")
  return PyBool_FromLong(isAllConstant(selfObject, NLLogicValue::Zero));
}

static PyObject* PySNLNet_isConstant1(PySNLNet* self) {
  METHOD_HEAD("SNLNet.isConstant1()")
  return PyBool_FromLong(isAllConstant(selfObject, NLLogicValue::One));
}

static PyObject* PySNLNet_isConstant(PySNLNet* self) {
  METHOD_HEAD("SNLNet.isConstant()")
  bool hasBits = false;
  for (auto* bit: selfObject->getBits()) {
    hasBits = true;
    if (!SNLDesignModeling::getConstantValue(bit)) return PyBool_FromLong(false);
  }
  return PyBool_FromLong(hasBits);
}

PyMethodDef PySNLNet_Methods[] = {
  { "getName", (PyCFunction)PySNLNet_getName, METH_NOARGS,
    "get SNLNet name"},
  { "getWidth", (PyCFunction)PySNLNet_getWidth, METH_NOARGS,
    "get SNLNet width"},
  { "getBits", (PyCFunction)PySNLNet_getBits, METH_NOARGS,
    "get a container of SNLBitNets."},
  { "isConstant0", (PyCFunction)PySNLNet_isConstant0, METH_NOARGS,
    "Returns True if this SNLNet is a Constant 0."},
  { "isConstant1", (PyCFunction)PySNLNet_isConstant1, METH_NOARGS,
    "Returns True if this SNLNet is a Constant 1."},
  { "isConstant", (PyCFunction)PySNLNet_isConstant, METH_NOARGS,
    "Returns True if this SNLNet is a Constant."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeNLAbstractObjectWithNLIDLinkPyType(SNLNet)

}
