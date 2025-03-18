// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_BIT_NET_H_
#define __PY_SNL_BIT_NET_H_

#include "PySNLNet.h"
#include "SNLBitNet.h"

namespace PYSNL {

typedef struct {
  PySNLNet parent_;
} PySNLBitNet;

extern PyTypeObject PyTypeSNLBitNet;

extern PyObject*    PySNLBitNet_Link(naja::SNL::SNLBitNet*);
extern void         PySNLBitNet_LinkPyType();

#define IsPySNLBitNet(v) (PyObject_TypeCheck(v, &PyTypeSNLBitNet))
#define PYSNLBitNet(v)   ((PySNLBitNet*)(v))
#define PYSNLBitNet_O(v) (static_cast<naja::SNL::SNLBitNet*>(PYSNLBitNet(v)->parent_.parent_.object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_BIT_NET_H_ */
