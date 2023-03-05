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

#ifndef __PY_SNL_NET_COMPONENT_H_
#define __PY_SNL_NET_COMPONENT_H_

#include "PySNLDesignObject.h"
#include "SNLNetComponent.h"

namespace PYSNL {

typedef struct {
  PySNLDesignObject parent_;
} PySNLNetComponent;

extern PyTypeObject PyTypeSNLNetComponent;

extern PyObject*    PySNLNetComponent_Link(naja::SNL::SNLNetComponent* u);
extern void         PySNLNetComponent_LinkPyType();

#define IsPySNLNetComponent(v) (PyObject_TypeCheck(v, &PyTypeSNLNetComponent))
#define PYSNLNetComponent(v)   ((PySNLNetComponent*)(v))
#define PYSNLNetComponent_O(v) (static_cast<SNL::SNLNetComponent*>(PYSNLNetComponent(v)->parent_->object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_NET_COMPONENT_H_ */
