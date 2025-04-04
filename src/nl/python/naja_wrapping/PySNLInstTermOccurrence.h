// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_INST_TREM_OCCURRENCE_H_
#define __PY_SNL_INST_TREM_OCCURRENCE_H_

#include <Python.h>
#include "SNLInstTermOccurrence.h"

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::SNLInstTermOccurrence* object_;
} PySNLInstTermOccurrence;

extern PyTypeObject PyTypeSNLInstTermOccurrence;

extern PyObject*  PySNLInstTermOccurrence_Link(const naja::NL::SNLInstTermOccurrence& occurrence);
extern void       PySNLInstTermOccurrence_LinkPyType();

#define IsPySNLInstTermOccurrence(v) (PyObject_TypeCheck(v, &PyTypeSNLInstTermOccurrence))
#define PYSNLInstTermOccurrence(v)   ((PySNLInstTermOccurrence*)(v))
#define PYSNLInstTermOccurrence_O(v) (PYSNLInstTermOccurrence(v)->object_)

} // PYNAJA namespace
 
#endif // __PY_SNL_INST_TREM_OCCURRENCE_H_