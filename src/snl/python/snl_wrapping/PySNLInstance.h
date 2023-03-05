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

#ifndef __PY_SNL_INSTANCE_H_
#define __PY_SNL_INSTANCE_H_

#include "PySNLDesignObject.h"

namespace naja::SNL {
  class SNLInstance;
}

namespace PYSNL {

typedef struct {
  PySNLDesignObject parent_;
} PySNLInstance;

extern PyTypeObject PyTypeSNLInstance;

extern PyObject*    PySNLInstance_Link(naja::SNL::SNLInstance* u);
extern void         PySNLInstance_LinkPyType();

#define IsPySNLInstance(v) (PyObject_TypeCheck(v, &PyTypeSNLInstance))
#define PYSNLInstance(v)   (static_cast<PySNLInstance*>(v))
#define PYSNLInstance_O(v) (static_cast<naja::SNL::SNLInstance*>(PYSNLInstance(v)->parent_->parent_->object_))

} // PYSNL namespace
 
#endif // __PY_SNL_INSTANCE_H_
