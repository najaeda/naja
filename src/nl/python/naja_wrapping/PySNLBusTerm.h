// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "NajaPythonExport.h"
#include "PySNLTerm.h"

namespace naja::NL {
  class SNLBusTerm;
}

namespace PYNAJA {

typedef struct {
  PySNLTerm parent_;
} PySNLBusTerm;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBusTerm;

extern PyObject*    PySNLBusTerm_Link(naja::NL::SNLBusTerm* t);
extern void         PySNLBusTerm_LinkPyType();

#define IsPySNLBusTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLBusTerm))
#define PYSNLBusTerm(v)   ((PySNLBusTerm*)(v))
#define PYSNLBusTerm_O(v) (static_cast<naja::NL::SNLBusTerm*>(PYSNLBusTerm(v)->parent_.parent_.parent_.object_))

} // PYNAJA namespace
 
