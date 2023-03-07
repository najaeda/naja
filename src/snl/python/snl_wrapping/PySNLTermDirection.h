/*
 * Copyright 2023 The Naja Authors.
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

#ifndef __PY_SNL_TERM_DIRECTION_H_
#define __PY_SNL_TERM_DIRECTION_H_

#include "PyInterface.h"
#include "SNLTerm.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLTerm::Direction* object_;
} PySNLTermDirection;

extern PyTypeObject PyTypeSNLTermDirection;

extern PyObject*    PySNLTermDirection_Link(naja::SNL::SNLTerm::Direction*);
extern void         PySNLTermDirection_LinkPyType();
extern void         PySNLTermDirection_postModuleInit();


#define IsPySNLTermDirection(v) (PyObject_TypeCheck(v, &PyTypeSNLTermDirection))
#define PYSNLTermDirection(v)   (static_cast<PySNLTermDirection*>(v))
#define PYSNLTermDirection_O(v) (static_cast<naja::SNL::SNLTerm::Direction*>(PYSNLTermDirection(v)->object_))

} /* PYSNL namespace */

#endif /* __PY_SNL_TERM_DIRECTION_H_ */