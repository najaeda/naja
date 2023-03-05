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

#ifndef __PY_SNL_BUS_NET_BIT_H_
#define __PY_SNL_BUS_NET_BIT_H_

#include "PySNLBitNet.h"
#include "SNLBusNetBit.h"

namespace PYSNL {

typedef struct {
  PySNLBitNet parent_;
} PySNLBusNetBit;

extern PyTypeObject PyTypeSNLBusNetBit;

extern PyObject*    PySNLBusNetBit_Link(naja::SNL::SNLBusNetBit* n);
extern void         PySNLBusNetBit_LinkPyType();

#define IsPySNLBusNetBit(v)  (PyObject_TypeCheck(v, &PyTypeSNLBusNetBit))
#define PYSNLBusNetBit(v)    (static_cast<PySNLBusNetBit*>(v))
#define PYSNLBusNetBit_O(v)  (PYSNLBusNetBit(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_BUS_NET_BIT_H_ */
