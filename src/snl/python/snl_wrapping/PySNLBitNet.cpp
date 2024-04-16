// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBitNet.h"

#include "PyInterface.h"
#include "PySNLScalarNet.h"
#include "PySNLBusNetBit.h"
#include "PySNLNetComponents.h"
#include "PySNLInstTerms.h"
#include "PySNLBitTerms.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_->parent_)
#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLBitNet, function)

GetContainerMethod(BitNet, NetComponent, NetComponents, Components)
GetContainerMethod(BitNet, InstTerm, InstTerms, InstTerms)
GetContainerMethod(BitNet, BitTerm, BitTerms, BitTerms)
GetBoolAttribute(BitNet, isConstant0)
GetBoolAttribute(BitNet, isConstant1)
GetBoolAttribute(BitNet, isConstant)

static PyObject* PySNLBitNet_getType(PySNLBitNet* self) {
  METHOD_HEAD("Net.getType()")
  return PyLong_FromLong((long)selfObject->getType());
}

static PyObject* PySNLBitNet_getTypeAsString(PySNLBitNet* self) {
  METHOD_HEAD("Net.getTypeAsString()")
  switch (selfObject->getType()) {
    case SNLNet::Type::Standard: return PyUnicode_FromString("Standard");
    case SNLNet::Type::Assign0: return PyUnicode_FromString("Assign0");
    case SNLNet::Type::Assign1: return PyUnicode_FromString("Assign1");
    case SNLNet::Type::Supply0: return PyUnicode_FromString("Supply0");
    case SNLNet::Type::Supply1: return PyUnicode_FromString("Supply1");
    default: return PyUnicode_FromString("Unknown");
  }
}

PyMethodDef PySNLBitNet_Methods[] = {
  { "getType", (PyCFunction)PySNLBitNet_getType, METH_NOARGS,
    "get the type of this Net."},
  { "getTypeAsString", (PyCFunction)PySNLBitNet_getTypeAsString, METH_NOARGS,
    "get the type of this Net as a string."},
  { "isConstant0", (PyCFunction)PySNLBitNet_isConstant0, METH_NOARGS,
    "Returns True if this SNLBitNet is a Constant 0."},
  { "isConstant1", (PyCFunction)PySNLBitNet_isConstant1, METH_NOARGS,
    "Returns True if this SNLBitNet is a Constant 1."},
  { "isConstant", (PyCFunction)PySNLBitNet_isConstant, METH_NOARGS,
    "Returns True if this SNLBitNet is a Constant."},
  { "getComponents", (PyCFunction)PySNLBitNet_getComponents, METH_NOARGS,
    "get a container of Net Components."},
  { "getInstTerms", (PyCFunction)PySNLBitNet_getInstTerms, METH_NOARGS,
    "get a container of Net InstTerms."},
  { "getBitTerms", (PyCFunction)PySNLBitNet_getBitTerms, METH_NOARGS,
    "get a container of Net BitTerms."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyObject* PySNLBitNet_Link(SNLBitNet* object) {
  if (not object) {
    Py_RETURN_NONE;   
  }
  if (auto busNetBit = dynamic_cast<SNLBusNetBit*>(object)) {
    return PySNLBusNetBit_Link(busNetBit);
  } else {
    auto scalarNet = static_cast<SNLScalarNet*>(object);
    return PySNLScalarNet_Link(scalarNet);
  }
}

PyTypeSNLAbstractObjectWithSNLIDLinkPyType(SNLBitNet)
PyTypeInheritedObjectDefinitions(SNLBitNet, SNLNet)

}