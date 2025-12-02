// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_PATH_H_
#define __PY_SNL_PATH_H_

#include <Python.h>
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLPath;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::SNLPath* object_;
} PySNLPath;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLPath;

extern PyObject*  PySNLPath_Link(const naja::NL::SNLPath& path);
extern void       PySNLPath_LinkPyType();

#define IsPySNLPath(v) (PyObject_TypeCheck(v, &PyTypeSNLPath))
#define PYSNLPath(v)   ((PySNLPath*)(v))
#define PYSNLPath_O(v) (PYSNLPath(v)->object_)

} // PYNAJA namespace
 
#endif // __PY_SNL_INSTANCE_H_