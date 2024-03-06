// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_UNIVERSE_H_
#define __PY_SNL_UNIVERSE_H_

#include <Python.h>
#include "SNLUniverse.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLUniverse* object_;
} PySNLUniverse;

extern PyTypeObject PyTypeSNLUniverse;

extern PyObject*    PySNLUniverse_Link(naja::SNL::SNLUniverse* u);
extern void         PySNLUniverse_LinkPyType();

#define IsPySNLUniverse(v) (PyObject_TypeCheck(v, &PyTypeSNLUniverse))
#define PYSNLUNIVERSE(v)   ((PySNLUniverse*)(v))
#define PYSNLUNIVERSE_O(v) (PYSNLUNIVERSE(v)->object_)

} // PYSNL namespace
 
#endif // __PY_SNL_UNIVERSE_H_
