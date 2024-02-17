// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_INST_PARAMETERS_H_
#define __PY_SNL_INST_PARAMETERS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLInstParameter;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLInstParameter*>* object_;
} PySNLInstParameters;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLInstParameter*>::Iterator* object_;
  PySNLInstParameters* container_;
} PySNLInstParametersIterator;

extern PyTypeObject PyTypeSNLInstParameters;
extern PyTypeObject PyTypeSNLInstParametersIterator;

extern void PySNLInstParameters_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_INST_PARAMETERS_H_ */
