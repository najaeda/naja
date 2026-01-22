// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaCollection.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLInstance;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLInstance*>* object_;
} PySNLInstances;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLInstance*>::Iterator* object_;
  PySNLInstances* container_;
} PySNLInstancesIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLInstances;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLInstancesIterator;

extern void PySNLInstances_LinkPyType();

} /* PYNAJA namespace */
 
