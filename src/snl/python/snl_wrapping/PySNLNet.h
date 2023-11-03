// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_NET_H_
#define __PY_SNL_NET_H_

#include "PySNLDesignObject.h"

namespace naja::SNL {
  class SNLNet;
}

namespace PYSNL {

typedef struct {
  PySNLDesignObject parent_;
} PySNLNet;

extern PyTypeObject PyTypeSNLNet;

extern PyObject*    PySNLNet_Link(naja::SNL::SNLNet*);
extern void         PySNLNet_LinkPyType();

#define IsPySNLNet(v) (PyObject_TypeCheck(v, &PyTypeSNLNet))
#define PYSNLNet(v)   ((PySNLNet*)(v))
#define PYSNLNet_O(v) (static_cast<naja::SNL::SNLNet*>(PYSNLNet(v)->parent_.object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_NET_H_ */
