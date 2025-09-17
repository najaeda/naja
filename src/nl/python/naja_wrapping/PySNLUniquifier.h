// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_UNIQUIFIER_H_
#define __PY_SNL_UNIQUIFIER_H_

#include <Python.h>
#include "SNLUniquifier.h"

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::SNLUniquifier* object_;
} PySNLUniquifier;

extern PyTypeObject PyTypeSNLUniquifier;

extern PyObject*  PySNLUniquifier_Link(const naja::NL::SNLUniquifier& uniquifier);
extern void       PySNLUniquifier_LinkPyType();

#define IsPySNLUniquifier(v) (PyObject_TypeCheck(v, &PyTypeSNLUniquifier))
#define PYSNLUniquifier(v)   ((PySNLUniquifier*)(v))
#define PYSNLUniquifier_O(v) (PYSNLUniquifier(v)->object_)

} // PYNAJA namespace
 
#endif // __PY_SNL_UNIQUIFIER_H_
