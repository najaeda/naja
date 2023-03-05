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

#ifndef __PY_SNL_TERM_H_
#define __PY_SNL_TERM_H_

#include "PySNLNetComponent.h"
#include "SNLTerm.h"

namespace PYSNL {

typedef struct {
  PySNLNetComponent parent_;
} PySNLTerm;

extern PyTypeObject PyTypeSNLTerm;
extern PyMethodDef  PySNLTerm_Methods[];

extern PyObject* PySNLTerm_Link(naja::SNL::SNLTerm*);
extern void PySNLTerm_LinkPyType();
extern void PySNLTerm_postModuleInit();

#define IsPySNLTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLTerm))
#define PYSNLTerm(v)   (static_cast<PySNLTerm*>(v))
#define PYSNLTerm_O(v) (static_cast<naja::SNL::SNLTerm*>(PYSNLTerm(v)->parent_->parent_->object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_TERM_H_ */
