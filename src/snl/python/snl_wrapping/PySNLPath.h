// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_PATH_H_
#define __PY_SNL_PATH_H_

#include <Python.h>
#include "PySNLPath.h"
#include "SNLPath.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLPath* object_;
} PySNLPath;

extern PyTypeObject PyTypeSNLPath;

extern PyObject*    PySNLPath_Link(naja::SNL::SNLPath* u);
extern void         PySNLPath_LinkPyType();

#define IsPySNLPath(v) (PyObject_TypeCheck(v, &PyTypeSNLPath))
#define PYSNLPath(v)   ((PySNLPath*)(v))
#define PYSNLPath_O(v) (PYSNLPath(v)->object_)

} // PYSNL namespace
 
#endif // __PY_SNL_INSTANCE_H_
