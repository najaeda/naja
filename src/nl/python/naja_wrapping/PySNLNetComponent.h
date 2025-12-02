// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_NET_COMPONENT_H_
#define __PY_SNL_NET_COMPONENT_H_

#include "PySNLDesignObject.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLNetComponent;
}

namespace PYNAJA {

typedef struct {
  PySNLDesignObject parent_;
} PySNLNetComponent;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLNetComponent;

extern PyObject*    PySNLNetComponent_Link(naja::NL::SNLNetComponent* u);
extern void         PySNLNetComponent_LinkPyType();

#define IsPySNLNetComponent(v) (PyObject_TypeCheck(v, &PyTypeSNLNetComponent))
#define PYSNLNetComponent(v)   ((PySNLNetComponent*)(v))
#define PYSNLNetComponent_O(v) (static_cast<naja::NL::SNLNetComponent*>(PYSNLNetComponent(v)->parent_.object_))

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_NET_COMPONENT_H_ */
