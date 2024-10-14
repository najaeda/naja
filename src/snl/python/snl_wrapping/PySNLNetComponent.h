// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_NET_COMPONENT_H_
#define __PY_SNL_NET_COMPONENT_H_

#include "PySNLDesignObject.h"
#include "SNLNetComponent.h"

namespace PYSNL {

typedef struct {
  PySNLDesignObject parent_;
} PySNLNetComponent;

extern PyTypeObject PyTypeSNLNetComponent;

extern PyObject*    PySNLNetComponent_Link(naja::SNL::SNLNetComponent* u);
extern void         PySNLNetComponent_LinkPyType();

#define IsPySNLNetComponent(v) (PyObject_TypeCheck(v, &PyTypeSNLNetComponent))
#define PYSNLNetComponent(v)   ((PySNLNetComponent*)(v))
#define PYSNLNetComponent_O(v) (static_cast<naja::SNL::SNLNetComponent*>(PYSNLNetComponent(v)->parent_.object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_NET_COMPONENT_H_ */
