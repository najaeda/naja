// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_NL_LIBRARIES_H_
#define __PY_NL_LIBRARIES_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class NLLibrary;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::NLLibrary*>* object_;
} PyNLLibraries;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::NLLibrary*>::Iterator* object_;
  PyNLLibraries* container_;
} PyNLLibrariesIterator;

extern PyTypeObject PyTypeNLLibraries;
extern PyTypeObject PyTypeNLLibrariesIterator;

extern void PyNLLibraries_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_NL_LIBRARIES_H_ */
