/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PySNLNet.h"

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

PyMethodDef PySNLNet_Methods[] = {
  { "getName", (PyCFunction)PySNLNet_getName, METH_NOARGS,
    "get SNLNet name"},
  { "getBits", (PyCFunction)PySNLNet_getBits, METH_NOARGS,
    "get a container of SNLBitNets."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLNet_destroy, PySNLNet)
DBoDeallocMethod(SNLNet)

DBoLinkCreateMethod(SNLNet)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLNet)
PyTypeInheritedObjectDefinitions(SNLNet, SNLDesignObject)

}
