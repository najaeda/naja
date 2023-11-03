// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_LIBRARIES_H_
#define __PY_SNL_LIBRARIES_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLLibrary;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLLibrary*>* object_;
} PySNLLibraries;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLLibrary*>::Iterator* object_;
  PySNLLibraries* container_;
} PySNLLibrariesIterator;

extern PyTypeObject PyTypeSNLLibraries;
extern PyTypeObject PyTypeSNLLibrariesIterator;

extern void PySNLLibraries_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_LIBRARIES_H_ */
