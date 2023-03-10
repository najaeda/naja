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

#ifndef __PY_SNL_UNIVERSE_H_
#define __PY_SNL_UNIVERSE_H_

#include "PyInterface.h"
#include "SNLUniverse.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLUniverse* object_;
} PySNLUniverse;

extern PyTypeObject PyTypeSNLUniverse;

extern PyObject*    PySNLUniverse_Link(naja::SNL::SNLUniverse* u);
extern void         PySNLUniverse_LinkPyType();

#define IsPySNLUniverse(v) (PyObject_TypeCheck(v, &PyTypeSNLUniverse))
#define PYSNLUNIVERSE(v)   ((PySNLUniverse*)(v))
#define PYSNLUNIVERSE_O(v) (PYSNLUNIVERSE(v)->object_)

} // PYSNL namespace
 
#endif // __PY_SNL_UNIVERSE_H_