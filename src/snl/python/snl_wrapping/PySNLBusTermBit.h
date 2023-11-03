// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_BUS_TERM_BIT_H_
#define __PY_SNL_BUS_TERM_BIT_H_

#include "PySNLBitTerm.h"
#include "SNLBusTermBit.h"

namespace PYSNL {

typedef struct {
  PySNLBitTerm parent_;
} PySNLBusTermBit;

extern PyTypeObject PyTypeSNLBusTermBit;

extern PyObject*    PySNLBusTermBit_Link(naja::SNL::SNLBusTermBit* t);
extern void         PySNLBusTermBit_LinkPyType();

#define IsPySNLBusTermBit(v)  (PyObject_TypeCheck(v, &PyTypeSNLBusTermBit))
#define PYSNLBusTermBit(v)    (static_cast<PySNLBusTermBit*>(v))
#define PYSNLBusTermBit_O(v)  (PYSNLBusTermBit(v)->object_)

} // PYSNL namespace
 
#endif // __PY_SNL_BUS_TERM_BIT_H_