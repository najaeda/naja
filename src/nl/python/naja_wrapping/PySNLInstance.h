// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_INSTANCE_H_
#define __PY_SNL_INSTANCE_H_

#include "PySNLDesignObject.h"
#include "SNLInstance.h"

namespace PYSNL {

typedef struct {
  PySNLDesignObject parent_;
} PySNLInstance;

extern PyTypeObject PyTypeSNLInstance;

extern PyObject*    PySNLInstance_Link(naja::SNL::SNLInstance* u);
extern void         PySNLInstance_LinkPyType();

#define IsPySNLInstance(v) (PyObject_TypeCheck(v, &PyTypeSNLInstance))
#define PYSNLInstance(v)   ((PySNLInstance*)(v))
#define PYSNLInstance_O(v) (static_cast<naja::SNL::SNLInstance*>(PYSNLInstance(v)->parent_.object_))

} // PYSNL namespace
 
#endif // __PY_SNL_INSTANCE_H_
