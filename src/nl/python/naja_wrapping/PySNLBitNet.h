// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "PySNLNet.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLBitNet;
}

namespace PYNAJA {

typedef struct {
  PySNLNet parent_;
} PySNLBitNet;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBitNet;

extern PyObject*    PySNLBitNet_Link(naja::NL::SNLBitNet*);
extern void         PySNLBitNet_LinkPyType();

#define IsPySNLBitNet(v) (PyObject_TypeCheck(v, &PyTypeSNLBitNet))
#define PYSNLBitNet(v)   ((PySNLBitNet*)(v))
#define PYSNLBitNet_O(v) (static_cast<naja::NL::SNLBitNet*>(PYSNLBitNet(v)->parent_.parent_.object_))

} /* PYNAJA namespace */
 
