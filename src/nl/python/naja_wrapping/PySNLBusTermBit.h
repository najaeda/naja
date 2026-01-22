// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "NajaPythonExport.h"
#include "PySNLBitTerm.h"

namespace naja::NL {
  class SNLBusTermBit;
}

namespace PYNAJA {

typedef struct {
  PySNLBitTerm parent_;
} PySNLBusTermBit;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBusTermBit;

extern PyObject*    PySNLBusTermBit_Link(naja::NL::SNLBusTermBit* t);
extern void         PySNLBusTermBit_LinkPyType();

#define IsPySNLBusTermBit(v)  (PyObject_TypeCheck(v, &PyTypeSNLBusTermBit))
#define PYSNLBusTermBit(v)    (static_cast<PySNLBusTermBit*>(v))
#define PYSNLBusTermBit_O(v)  (PYSNLBusTermBit(v)->object_)

} // PYNAJA namespace
 
