// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_PARAMETERS_H_
#define __PY_SNL_PARAMETERS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLParameter;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLParameter*>* object_;
} PySNLParameters;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLParameter*>::Iterator* object_;
  PySNLParameters* container_;
} PySNLParametersIterator;

extern PyTypeObject PyTypeSNLParameters;
extern PyTypeObject PyTypeSNLParametersIterator;

extern void PySNLParameters_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_PARAMETERS_H_ */
