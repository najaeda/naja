// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_ATTRIBUTE_H_
#define __PY_SNL_ATTRIBUTE_H_

#include <Python.h>
#include "NajaPythonExport.h"
#include "SNLAttributes.h"

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::SNLAttribute* object_;
} PySNLAttribute;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLAttribute;

extern PyObject*  PySNLAttribute_Link(const naja::NL::SNLAttribute& attribute);
extern void       PySNLAttribute_LinkPyType();

#define IsPySNLAttribute(v) (PyObject_TypeCheck(v, &PyTypeSNLAttribute))
#define PYSNLAttribute(v)   ((PySNLAttribute*)(v))
#define PYSNLAttribute_O(v) (PYSNLAttribute(v)->object_)

} // PYNAJA namespace
 
#endif // __PY_SNL_ATTRIBUTE_H_