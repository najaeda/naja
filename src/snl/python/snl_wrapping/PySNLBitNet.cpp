// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBitNet.h"
#include "PyInterface.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(BitNet, net, function)

PyMethodDef PySNLBitNet_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLBitNet)

DBoLinkCreateMethod(SNLBitNet)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLBitNet)
PyTypeInheritedObjectDefinitions(SNLBitNet, SNLNet)

}
