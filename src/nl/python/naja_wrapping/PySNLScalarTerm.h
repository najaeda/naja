// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "PySNLBitTerm.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLScalarTerm;
}

namespace PYNAJA {

typedef struct {
  PySNLBitTerm parent_;
} PySNLScalarTerm;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLScalarTerm;

extern PyObject*    PySNLScalarTerm_Link(naja::NL::SNLScalarTerm* t);
extern void         PySNLScalarTerm_LinkPyType();

#define IsPySNLScalarTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLScalarTerm))
#define PYSNLScalarTerm(v)   (static_cast<PySNLScalarTerm*>(v))
#define PYSNLScalarTerm_O(v) (PYSNLScalarTerm(v)->object_)

} // PYNAJA namespace
 
