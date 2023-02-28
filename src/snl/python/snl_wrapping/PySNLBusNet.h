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

#ifndef __PY_SNL_BUS_NET_H_
#define __PY_SNL_BUS_NET_H_

#include "PySNLNet.h"

#include "SNLBusNet.h"

namespace PYSNL {

typedef struct {
  PySNLNet parent_;
} PySNLBusNet;

extern PyTypeObject PyTypeSNLBusNet;
extern PyMethodDef  PySNLBusNet_Methods[];

extern PyObject*    PySNLBusNet_Link(naja::SNL::SNLBusNet* t);
extern void         PySNLBusNet_LinkPyType();

#define IsPySNLBusNet(v)  (PyObject_TypeCheck(v, &PyTypeSNLBusNet))
#define PYSNLBusNet(v)    ((PySNLBusNet*)(v))
#define PYSNLBusNet_O(v)  (PYSNLBusNet(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_BUS_NET_H_ */
