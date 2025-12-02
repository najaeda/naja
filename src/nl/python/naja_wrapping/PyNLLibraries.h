// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_NL_LIBRARIES_H_
#define __PY_NL_LIBRARIES_H_

#include <Python.h>
#include "NajaPythonExport.h"
#include "NajaCollection.h"

namespace naja::NL {
  class NLLibrary;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::NLLibrary*>* object_;
} PyNLLibraries;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::NLLibrary*>::Iterator* object_;
  PyNLLibraries* container_;
} PyNLLibrariesIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeNLLibraries;
NAJA_PY_EXPORT extern PyTypeObject PyTypeNLLibrariesIterator;

extern void PyNLLibraries_LinkPyType();

} /* PYNAJA namespace */
 
#endif /* __PY_NL_LIBRARIES_H_ */
