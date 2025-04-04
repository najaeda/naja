// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_PARAMETERS_H_
#define __PY_SNL_PARAMETERS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::NL {
  class SNLParameter;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLParameter*>* object_;
} PySNLParameters;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLParameter*>::Iterator* object_;
  PySNLParameters* container_;
} PySNLParametersIterator;

extern PyTypeObject PyTypeSNLParameters;
extern PyTypeObject PyTypeSNLParametersIterator;

extern void PySNLParameters_LinkPyType();

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_PARAMETERS_H_ */