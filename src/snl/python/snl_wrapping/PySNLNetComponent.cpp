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

#include "PySNLNetComponent.h"

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

PyMethodDef PySNLNetComponent_Methods[] = {
  { "getNet", (PyCFunction)PySNLNetComponent_getNet, METH_NOARGS,
    "get SNLNetComponent SNLBitNet"},
  { "setNet", (PyCFunction)PySNLNetComponent_setNet, METH_O,
    "set SNLNetComponent SNLNet"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLNetComponent_destroy, PySNLNetComponent)
DBoDeallocMethod(SNLNetComponent)

DBoLinkCreateMethod(SNLNetComponent)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLNetComponent)
PyTypeInheritedObjectDefinitions(SNLNetComponent, SNLDesignObject)

}