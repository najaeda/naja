// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "PySNLBitNet.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLBusNetBit;
}

namespace PYNAJA {

typedef struct {
  PySNLBitNet parent_;
} PySNLBusNetBit;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBusNetBit;

extern PyObject*    PySNLBusNetBit_Link(naja::NL::SNLBusNetBit* n);
extern void         PySNLBusNetBit_LinkPyType();

#define IsPySNLBusNetBit(v)  (PyObject_TypeCheck(v, &PyTypeSNLBusNetBit))
#define PYSNLBusNetBit(v)    (static_cast<PySNLBusNetBit*>(v))
#define PYSNLBusNetBit_O(v)  (PYSNLBusNetBit(v)->object_)

} /* PYNAJA namespace */
 
