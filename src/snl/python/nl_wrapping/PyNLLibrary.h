// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_NL_LIBRARY_H_
#define __PY_NL_LIBRARY_H_

#include <Python.h>

namespace naja::SNL {
  class NLLibrary;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::NLLibrary* object_;
} PyNLLibrary;

extern PyTypeObject PyTypeNLLibrary;

extern PyObject*    PyNLLibrary_Link(naja::SNL::NLLibrary* u);
extern void         PyNLLibrary_LinkPyType();

#define IsPyNLLibrary(v) (PyObject_TypeCheck(v, &PyTypeNLLibrary))
#define PYNLLibrary(v)   ((PyNLLibrary*)(v))
#define PYNLLibrary_O(v) (PYNLLibrary(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_NL_LIBRARY_H_ */
