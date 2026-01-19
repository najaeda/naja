// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "PySNLTerm.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLBitTerm;
}

namespace PYNAJA {

typedef struct {
  PySNLTerm parent_;
} PySNLBitTerm;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBitTerm;

extern PyObject*    PySNLBitTerm_Link(naja::NL::SNLBitTerm*);
extern void         PySNLBitTerm_LinkPyType();

#define IsPySNLBitTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLBitTerm))
#define PYSNLBitTerm(v)   ((PySNLBitTerm*)(v))
#define PYSNLBitTerm_O(v) (static_cast<naja::NL::SNLBitTerm*>(PYSNLBitTerm(v)->parent_.parent_.parent_.object_))

} /* PYNAJA namespace */
 
