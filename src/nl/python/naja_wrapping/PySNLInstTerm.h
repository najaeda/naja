// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "PySNLNetComponent.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLInstTerm;
}

namespace PYNAJA {

typedef struct {
  PySNLNetComponent parent_;
} PySNLInstTerm;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLInstTerm;

extern PyObject* PySNLInstTerm_Link(naja::NL::SNLInstTerm*);
extern void PySNLInstTerm_LinkPyType();
extern void PySNLInstTerm_postModuleInit();

#define IsPySNLInstTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLInstTerm))
#define PYSNLInstTerm(v)   ((PySNLInstTerm*)(v))
#define PYSNLInstTerm_O(v) (static_cast<naja::NL::SNLInstTerm*>(PYSNLInstTerm(v)->parent_.parent_.object_))

} /* PYNAJA namespace */
 
