// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_INST_PARAMETERS_H_
#define __PY_SNL_INST_PARAMETERS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::NL {
  class SNLInstParameter;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLInstParameter*>* object_;
} PySNLInstParameters;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLInstParameter*>::Iterator* object_;
  PySNLInstParameters* container_;
} PySNLInstParametersIterator;

extern PyTypeObject PyTypeSNLInstParameters;
extern PyTypeObject PyTypeSNLInstParametersIterator;

extern void PySNLInstParameters_LinkPyType();

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_INST_PARAMETERS_H_ */
