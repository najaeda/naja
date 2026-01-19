// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PySNLDesignObject.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLNet;
}

namespace PYNAJA {

typedef struct {
  PySNLDesignObject parent_;
} PySNLNet;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLNet;

extern PyObject*    PySNLNet_Link(naja::NL::SNLNet*);
extern void         PySNLNet_LinkPyType();
extern void         PySNLNet_postModuleInit();

#define IsPySNLNet(v) (PyObject_TypeCheck(v, &PyTypeSNLNet))
#define PYSNLNet(v)   ((PySNLNet*)(v))
#define PYSNLNet_O(v) (static_cast<naja::NL::SNLNet*>(PYSNLNet(v)->parent_.object_))

} /* PYNAJA namespace */
 