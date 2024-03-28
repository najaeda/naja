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

PyMethodDef PySNLBitNet_Methods[] = {
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


PyTypeSNLObjectWithSNLIDLinkPyType(SNLBitNet)
PyTypeInheritedObjectDefinitions(SNLBitNet, SNLNet)

}