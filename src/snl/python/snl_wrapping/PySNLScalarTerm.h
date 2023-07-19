// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_SCALAR_TERM_H_
#define __PY_SNL_SCALAR_TERM_H_

#include "PySNLBitTerm.h"
#include "SNLScalarTerm.h"

namespace PYSNL {

typedef struct {
  PySNLBitTerm parent_;
} PySNLScalarTerm;

extern PyTypeObject PyTypeSNLScalarTerm;

extern PyObject*    PySNLScalarTerm_Link(naja::SNL::SNLScalarTerm* t);
extern void         PySNLScalarTerm_LinkPyType();

#define IsPySNLScalarTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLScalarTerm))
#define PYSNLScalarTerm(v)   (static_cast<PySNLScalarTerm*>(v))
#define PYSNLScalarTerm_O(v) (PYSNLScalarTerm(v)->object_)

} // PYSNL namespace
 
#endif // __PY_SNL_SCALAR_TERM_H_
