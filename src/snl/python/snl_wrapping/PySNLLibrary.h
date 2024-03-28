// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_LIBRARY_H_
#define __PY_SNL_LIBRARY_H_

#include <Python.h>

namespace naja::SNL {
  class SNLLibrary;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLLibrary* object_;
} PySNLLibrary;

extern PyTypeObject PyTypeSNLLibrary;

extern PyObject*    PySNLLibrary_Link(naja::SNL::SNLLibrary* u);
extern void         PySNLLibrary_LinkPyType();

#define IsPySNLLibrary(v) (PyObject_TypeCheck(v, &PyTypeSNLLibrary))
#define PYSNLLibrary(v)   ((PySNLLibrary*)(v))
#define PYSNLLibrary_O(v) (PYSNLLibrary(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_LIBRARY_H_ */