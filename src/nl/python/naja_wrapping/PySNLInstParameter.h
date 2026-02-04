// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLInstParameter;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::SNLInstParameter* object_;
} PySNLInstParameter;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLInstParameter;

extern PyObject* PySNLInstParameter_Link(naja::NL::SNLInstParameter* u);
extern void PySNLInstParameter_LinkPyType();
extern void PySNLInstParameter_postModuleInit();

#define IsPySNLInstParameter(v) (PyObject_TypeCheck(v, &PyTypeSNLInstParameter))
#define PYSNLInstParameter(v)   ((PySNLInstParameter*)(v))
#define PYSNLInstParameter_O(v) (PYSNLInstParameter(v)->object_)

} /* PYNAJA namespace */
 
