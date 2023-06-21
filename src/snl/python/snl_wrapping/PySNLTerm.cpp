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

#include "PySNLTerm.h"

#include "PyInterface.h"
#include "PySNLTermDirection.h"
#include "PySNLDesign.h"
#include "PySNLBitTerms.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(SNLTerm, function)

GetNameMethod(SNLTerm)
GetContainerMethodWithMethodName(Term, BitTerm, getBits)

PyMethodDef PySNLTerm_Methods[] = {
  { "getName", (PyCFunction)PySNLTerm_getName, METH_NOARGS,
    "get SNLNet name"},
  { "getBits", (PyCFunction)PySNLTerm_getBits, METH_NOARGS,
    "get a container of SNLBitTerms."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLTerm_destroy, PySNLTerm)
DBoDeallocMethod(SNLTerm)

DBoLinkCreateMethod(SNLTerm)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLTerm)
PyTypeInheritedObjectDefinitions(SNLTerm, SNLNetComponent)

void PySNLTerm_postModuleInit() {
  PySNLTermDirection_postModuleInit();
  PyDict_SetItemString(PyTypeSNLTerm.tp_dict, "Direction", (PyObject*)&PyTypeSNLTermDirection);
}

}
