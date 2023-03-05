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

#include "PySNLTermDirection.h"

namespace PYSNL {

using namespace naja::SNL;

PyMethodDef PySNLTermDirection_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeObjectDefinitions(SNLTermDirection)

extern void PySNLTermDirection_postModuleInit() {
  PyObject* constant;
  LoadObjectConstant(PyTypeSNLTermDirection.tp_dict, SNLTerm::Direction::Input,   "Input");
  LoadObjectConstant(PyTypeSNLTermDirection.tp_dict, SNLTerm::Direction::Output,  "Output");
  LoadObjectConstant(PyTypeSNLTermDirection.tp_dict, SNLTerm::Direction::InOut,   "InOut");
}

//PyTypeObjectLinkPyType(SNLTermDirection)

extern void PySNLTermDirection_LinkPyType() {
  //`PyTypeSNLTermDirection.tp_dealloc     = (destructor) PyNetDirection_DeAlloc;
  //PyTypeSNLTermDirection.tp_richcompare = (richcmpfunc)PyNetDirection_Cmp;
  //PyTypeSNLTermDirection.tp_repr        = (reprfunc)   PyNetDirection_Repr;
  //PyTypeSNLTermDirection.tp_str         = (reprfunc)   PyNetDirection_Str;
  //PyTypeSNLTermDirection.tp_hash        = (hashfunc)   PyNetDirection_Hash;
  PyTypeSNLTermDirection.tp_methods = PySNLTermDirection_Methods;

}


#if 0

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(Term, term, function)

PyMethodDef PySNLTerm_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLTerm_destroy, PySNLTerm)
DBoDeallocMethod(SNLTerm)

DBoLinkCreateMethod(SNLTerm)
PyTypeObjectLinkPyType(SNLTerm)

#endif

}