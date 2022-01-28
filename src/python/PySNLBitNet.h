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

#ifndef __PY_SNL_BIT_NET_H_
#define __PY_SNL_BIT_NET_H_

#include "PySNLNet.h"
#include "SNLBitNet.h"

namespace PYSNL {

typedef struct {
  PySNLNet parent_;
} PySNLBitNet;

extern PyTypeObject PyTypeSNLBitNet;
extern PyMethodDef  PySNLBitNet_Methods[];

extern PyObject*    PySNLBitNet_Link(SNL::SNLBitNet*);
extern void         PySNLBitNet_LinkPyType();

#define IsPySNLBitNet(v) (PyObject_TypeCheck(v, &PyTypeSNLBitNet))
#define PYSNLBitNet(v)   ((PySNLBitNet*)(v))
#define PYSNLBitNet_O(v) (static_cast<SNL::SNLBitNet*>(PYSNLBitNet(v)->parent_.parent_.object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_BIT_NET_H_ */
