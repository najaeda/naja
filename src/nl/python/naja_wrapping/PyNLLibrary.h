// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaPythonExport.h"

namespace naja::NL {
  class NLLibrary;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::NLLibrary* object_;
} PyNLLibrary;

NAJA_PY_EXPORT extern PyTypeObject PyTypeNLLibrary;

extern PyObject*    PyNLLibrary_Link(naja::NL::NLLibrary* u);
extern void         PyNLLibrary_LinkPyType();

#define IsPyNLLibrary(v) (PyObject_TypeCheck(v, &PyTypeNLLibrary))
#define PYNLLibrary(v)   ((PyNLLibrary*)(v))
#define PYNLLibrary_O(v) (PYNLLibrary(v)->object_)

} /* PYNAJA namespace */
 
