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

#ifndef __PY_SNL_DESIGN_H_
#define __PY_SNL_DESIGN_H_

#include "PyInterface.h"

namespace naja::SNL {
  class SNLDesign;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLDesign* object_;
} PySNLDesign;

extern PyTypeObject PyTypeSNLDesign;
extern PyMethodDef  PySNLDesign_Methods[];

extern PyObject*    PySNLDesign_Link(naja::SNL::SNLDesign* u);
extern void         PySNLDesign_LinkPyType();

#define IsPySNLDesign(v) (PyObject_TypeCheck(v, &PyTypeSNLDesign))
#define PYSNLDesign(v)   ((PySNLDesign*)(v))
#define PYSNLDesign_O(v) (PYSNLDesign(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_DESIGN_H_ */
