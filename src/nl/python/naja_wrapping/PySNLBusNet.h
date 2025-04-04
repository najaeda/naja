// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_BUS_NET_H_
#define __PY_SNL_BUS_NET_H_

#include "PySNLNet.h"
#include "SNLBusNet.h"

namespace PYNAJA {

typedef struct {
  PySNLNet parent_;
} PySNLBusNet;

extern PyTypeObject PyTypeSNLBusNet;

extern PyObject*    PySNLBusNet_Link(naja::NL::SNLBusNet* t);
extern void         PySNLBusNet_LinkPyType();

#define IsPySNLBusNet(v)  (PyObject_TypeCheck(v, &PyTypeSNLBusNet))
#define PYSNLBusNet(v)    ((PySNLBusNet*)(v))
#define PYSNLBusNet_O(v)  (PYSNLBusNet(v)->object_)

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_BUS_NET_H_ */
