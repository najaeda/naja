// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaPythonExport.h"

namespace naja::NL {
  class NLClock;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::NLClock* object_;
} PyNLClock;

NAJA_PY_EXPORT extern PyTypeObject PyTypeNLClock;

extern PyObject* PyNLClock_Link(naja::NL::NLClock* u);
extern void PyNLClock_LinkPyType();
extern void PyNLClock_postModuleInit();

#define IsPyNLClock(v) (PyObject_TypeCheck(v, &PyTypeNLClock))
#define PYNLClock(v)   ((PyNLClock*)(v))
#define PYNLClock_O(v) (PYNLClock(v)->object_)

} /* PYNAJA namespace */
