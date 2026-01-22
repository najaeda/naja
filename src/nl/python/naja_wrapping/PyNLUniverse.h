// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaPythonExport.h"

namespace naja::NL {
  class NLUniverse;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::NLUniverse* object_;
} PyNLUniverse;

extern NAJA_PY_EXPORT PyTypeObject PyTypeNLUniverse;

extern PyObject*    PyNLUniverse_Link(naja::NL::NLUniverse* u);
extern void         PyNLUniverse_LinkPyType();

#define IsPyNLUniverse(v) (PyObject_TypeCheck(v, &PyTypeNLUniverse))
#define PYNLUNIVERSE(v)   ((PyNLUniverse*)(v))
#define PYNLUNIVERSE_O(v) (PYNLUNIVERSE(v)->object_)

} // PYNAJA namespace
 
