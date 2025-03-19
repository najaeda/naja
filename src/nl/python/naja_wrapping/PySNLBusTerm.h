// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_BUS_TERM_H_
#define __PY_SNL_BUS_TERM_H_

#include "PySNLTerm.h"
#include "SNLBusTerm.h"

namespace PYNAJA {

typedef struct {
  PySNLTerm parent_;
} PySNLBusTerm;

extern PyTypeObject PyTypeSNLBusTerm;

extern PyObject*    PySNLBusTerm_Link(naja::NL::SNLBusTerm* t);
extern void         PySNLBusTerm_LinkPyType();

#define IsPySNLBusTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLBusTerm))
#define PYSNLBusTerm(v)   ((PySNLBusTerm*)(v))
#define PYSNLBusTerm_O(v) (static_cast<naja::NL::SNLBusTerm*>(PYSNLBusTerm(v)->parent_.parent_.parent_.object_))

} // PYNAJA namespace
 
#endif // __PY_SNL_BUS_TERM_H_