// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_ATTRIBUTE_H_
#define __PY_SNL_ATTRIBUTE_H_

#include <Python.h>
#include "SNLAttributes.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLAttribute* object_;
} PySNLAttribute;

extern PyTypeObject PyTypeSNLAttribute;

extern PyObject*  PySNLAttribute_Link(const naja::SNL::SNLAttribute& attribute);
extern void       PySNLAttribute_LinkPyType();

#define IsPySNLAttribute(v) (PyObject_TypeCheck(v, &PyTypeSNLAttribute))
#define PYSNLAttribute(v)   ((PySNLAttribute*)(v))
#define PYSNLAttribute_O(v) (PYSNLAttribute(v)->object_)

} // PYSNL namespace
 
#endif // __PY_SNL_ATTRIBUTE_H_