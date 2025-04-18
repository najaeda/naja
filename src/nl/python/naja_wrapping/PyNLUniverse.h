// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_NL_UNIVERSE_H_
#define __PY_NL_UNIVERSE_H_

#include <Python.h>
#include "NLUniverse.h"

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::NLUniverse* object_;
} PyNLUniverse;

extern PyTypeObject PyTypeNLUniverse;

extern PyObject*    PyNLUniverse_Link(naja::NL::NLUniverse* u);
extern void         PyNLUniverse_LinkPyType();

#define IsPyNLUniverse(v) (PyObject_TypeCheck(v, &PyTypeNLUniverse))
#define PYNLUNIVERSE(v)   ((PyNLUniverse*)(v))
#define PYNLUNIVERSE_O(v) (PYNLUNIVERSE(v)->object_)

} // PYNAJA namespace
 
#endif // __PY_NL_UNIVERSE_H_
