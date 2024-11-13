// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_NET_COMPONENT_OCCURRENCE_H_
#define __PY_SNL_NET_COMPONENT_OCCURRENCE_H_

#include <Python.h>

namespace naja::SNL {
  class SNLNetComponentOccurrence;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLNetComponentOccurrence* object_;
} PySNLNetComponentOccurrence;

extern PyTypeObject PyTypeSNLNetComponentOccurrence;

extern PyObject*  PySNLNetComponentOccurrence_Link(const naja::SNL::SNLNetComponentOccurrence& occurrence);
extern void       PySNLNetComponentOccurrence_LinkPyType();

#define IsPySNLNetComponentOccurrence(v) (PyObject_TypeCheck(v, &PyTypeSNLNetComponentOccurrence))
#define PYSNLNetComponentOccurrence(v)   ((PySNLNetComponentOccurrence*)(v))
#define PYSNLNetComponentOccurrence_O(v) (PYSNLNetComponentOccurrence(v)->object_)

} // PYSNL namespace
 
#endif // __PY_SNL_NET_COMPONENT_OCCURRENCE_H_