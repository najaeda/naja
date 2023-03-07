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

#ifndef __PY_SNL_BUS_TERM_H_
#define __PY_SNL_BUS_TERM_H_

#include "PySNLTerm.h"

#include "SNLBusTerm.h"

namespace PYSNL {

typedef struct {
  PySNLTerm parent_;
} PySNLBusTerm;

extern PyTypeObject PyTypeSNLBusTerm;

extern PyObject*    PySNLBusTerm_Link(naja::SNL::SNLBusTerm* t);
extern void         PySNLBusTerm_LinkPyType();

#define IsPySNLBusTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLBusTerm))
#define PYSNLBusTerm(v)   ((PySNLBusTerm*)(v))
#define PYSNLBusTerm_O(v) (PYSNLBusTerm(v)->object_)

} // PYSNL namespace
 
#endif // __PY_SNL_BUS_TERM_H_