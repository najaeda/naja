// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_SCALAR_TERM_H_
#define __PY_SNL_SCALAR_TERM_H_

#include "PySNLBitTerm.h"
#include "SNLScalarTerm.h"

namespace PYNAJA {

typedef struct {
  PySNLBitTerm parent_;
} PySNLScalarTerm;

extern PyTypeObject PyTypeSNLScalarTerm;

extern PyObject*    PySNLScalarTerm_Link(naja::NL::SNLScalarTerm* t);
extern void         PySNLScalarTerm_LinkPyType();

#define IsPySNLScalarTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLScalarTerm))
#define PYSNLScalarTerm(v)   (static_cast<PySNLScalarTerm*>(v))
#define PYSNLScalarTerm_O(v) (PYSNLScalarTerm(v)->object_)

} // PYNAJA namespace
 
#endif // __PY_SNL_SCALAR_TERM_H_
