// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "PySNLNet.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLBusNet;
}

namespace PYNAJA {

typedef struct {
  PySNLNet parent_;
} PySNLBusNet;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBusNet;

extern PyObject*    PySNLBusNet_Link(naja::NL::SNLBusNet* t);
extern void         PySNLBusNet_LinkPyType();

#define IsPySNLBusNet(v)  (PyObject_TypeCheck(v, &PyTypeSNLBusNet))
#define PYSNLBusNet(v)    ((PySNLBusNet*)(v))
#define PYSNLBusNet_O(v)  (PYSNLBusNet(v)->object_)

} /* PYNAJA namespace */
 
