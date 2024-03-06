// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLNet.h"

#include "SNLNet.h"

#include "PyInterface.h"
#include "PySNLDesign.h"
#include "PySNLBitNets.h"

namespace PYSNL {

using namespace naja::SNL;

#undef ACCESS_OBJECT
#undef ACCESS_CLASS
#define ACCESS_OBJECT            parent_.object_
#define ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLNet, function)

GetNameMethod(SNLNet)
GetContainerMethodWithMethodName(Net, BitNet, getBits)

DBoDeallocMethod(SNLNet)

DBoLinkCreateMethod(SNLNet)
PyTypeInheritedObjectDefinitions(SNLNet, SNLDesignObject)

PyMethodDef PySNLNet_Methods[] = {
  { "getName", (PyCFunction)PySNLNet_getName, METH_NOARGS,
    "get SNLNet name"},
  { "getBits", (PyCFunction)PySNLNet_getBits, METH_NOARGS,
    "get a container of SNLBitNets."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeSNLObjectWithSNLIDLinkPyType(SNLNet)

}
