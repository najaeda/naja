// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaCollection.h"
#include "NajaPythonExport.h"

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

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLParameters;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLParametersIterator;

extern void PySNLParameters_LinkPyType();

} /* PYNAJA namespace */
 
