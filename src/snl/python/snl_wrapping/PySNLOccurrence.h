// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_OCCURRENCE_H_
#define __PY_SNL_OCCURRENCE_H_

#include <Python.h>

namespace naja::SNL {
  class SNLOccurrence;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLOccurrence* object_;
} PySNLOccurrence;

extern PyTypeObject PyTypeSNLOccurrence;

extern PyObject*  PySNLOccurrence_Link(naja::SNL::SNLOccurrence* occurrence);
extern void       PySNLOccurrence_LinkPyType();

#define IsPySNLOccurrence(v) (PyObject_TypeCheck(v, &PyTypeSNLOccurrence))
#define PYSNLOccurrence(v)   ((PySNLOccurrence*)(v))
#define PYSNLOccurrence_O(v) (PYSNLOccurrence(v)->object_)

} // PYSNL namespace
 
#endif // __PY_SNL_OCCURRENCE_H_