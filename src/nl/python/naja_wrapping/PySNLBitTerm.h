// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_BIT_TERM_H_
#define __PY_SNL_BIT_TERM_H_

#include "PySNLTerm.h"
#include "SNLBitTerm.h"

namespace PYNAJA {

typedef struct {
  PySNLTerm parent_;
} PySNLBitTerm;

extern PyTypeObject PyTypeSNLBitTerm;

extern PyObject*    PySNLBitTerm_Link(naja::NL::SNLBitTerm*);
extern void         PySNLBitTerm_LinkPyType();

#define IsPySNLBitTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLBitTerm))
#define PYSNLBitTerm(v)   ((PySNLBitTerm*)(v))
#define PYSNLBitTerm_O(v) (static_cast<naja::NL::SNLBitTerm*>(PYSNLBitTerm(v)->parent_.parent_.parent_.object_))

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_BIT_TERM_H_ */