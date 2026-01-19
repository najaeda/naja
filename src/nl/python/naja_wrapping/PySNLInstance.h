// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "PySNLDesignObject.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLInstance;
}

namespace PYNAJA {

typedef struct {
  PySNLDesignObject parent_;
} PySNLInstance;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLInstance;

extern PyObject*    PySNLInstance_Link(naja::NL::SNLInstance* u);
extern void         PySNLInstance_LinkPyType();

#define IsPySNLInstance(v) (PyObject_TypeCheck(v, &PyTypeSNLInstance))
#define PYSNLInstance(v)   ((PySNLInstance*)(v))
#define PYSNLInstance_O(v) (static_cast<naja::NL::SNLInstance*>(PYSNLInstance(v)->parent_.object_))

} // PYNAJA namespace
 
