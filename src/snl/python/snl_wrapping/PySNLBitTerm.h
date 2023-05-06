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

#ifndef __PY_SNL_BIT_TERM_H_
#define __PY_SNL_BIT_TERM_H_

#include "PySNLTerm.h"
#include "SNLBitTerm.h"

namespace PYSNL {

typedef struct {
  PySNLTerm parent_;
} PySNLBitTerm;

extern PyTypeObject PyTypeSNLBitTerm;

extern PyObject*    PySNLBitTerm_Link(naja::SNL::SNLBitTerm*);
extern void         PySNLBitTerm_LinkPyType();

#define IsPySNLBitTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLBitTerm))
#define PYSNLBitTerm(v)   (static_cast<PySNLBitTerm*>(v))
#define PYSNLBitTerm_O(v) (static_cast<naja::SNL::SNLBitTerm*>(PYSNLBitTerm(v)->parent_->parent_->parent_->object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_BIT_TERM_H_ */
