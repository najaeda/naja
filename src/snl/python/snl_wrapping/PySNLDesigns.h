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

#ifndef __PY_SNL_DESIGNS_H_
#define __PY_SNL_DESIGNS_H_

#include "NajaCollection.h"

namespace naja::SNL {
  class SNLDesign;
}

#include "PyInterface.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLDesign*>* object_;
} PySNLDesigns;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLDesign*>::Iterator* object_;
  PySNLDesigns* container_;
} PySNLDesignsIterator;

extern PyTypeObject PyTypeSNLDesigns;
extern PyTypeObject PyTypeSNLDesignsIterator;

extern void PySNLDesigns_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_DESIGNS_H_ */
