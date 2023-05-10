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

#ifndef __PY_SNL_DESIGN_OBJECT_H_
#define __PY_SNL_DESIGN_OBJECT_H_

#include "Python.h"
#include "SNLDesignObject.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLDesignObject* object_;
} PySNLDesignObject;

extern PyTypeObject PyTypeSNLDesignObject;

extern PyObject*    PySNLDesignObject_Link(naja::SNL::SNLDesignObject* u);
extern void         PySNLDesignObject_LinkPyType();

#define IsPySNLDesignObject(v) (PyObject_TypeCheck(v, &PyTypeSNLDesignObject))
#define PYSNLDesignObject(v)   ((PySNLDesignObject*)(v))
#define PYSNLDesignObject_O(v) (PYSNLDesignObject(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_DESIGN_OBJECT_H_ */
