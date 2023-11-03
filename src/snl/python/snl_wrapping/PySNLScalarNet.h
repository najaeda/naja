// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_SCALAR_NET_H_
#define __PY_SNL_SCALAR_NET_H_

#include "PySNLBitNet.h"
#include "SNLScalarNet.h"

namespace PYSNL {

typedef struct {
  PySNLBitNet parent_;
} PySNLScalarNet;

extern PyTypeObject PyTypeSNLScalarNet;

extern PyObject*    PySNLScalarNet_Link(naja::SNL::SNLScalarNet* n);
extern void         PySNLScalarNet_LinkPyType();

#define IsPySNLScalarNet(v) (PyObject_TypeCheck(v, &PyTypeSNLScalarNet))
#define PYSNLScalarNet(v)   (static_cast<PySNLScalarNet*>(v))
#define PYSNLScalarNet_O(v) (PYSNLScalarNet(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_SCALAR_NET_H_ */
