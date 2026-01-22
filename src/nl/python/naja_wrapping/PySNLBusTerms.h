// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaPythonExport.h"
#include "NajaCollection.h"

namespace naja::NL {
  class SNLBusTerm;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLBusTerm*>* object_;
} PySNLBusTerms;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLBusTerm*>::Iterator* object_;
  PySNLBusTerms* container_;
} PySNLBusTermsIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBusTerms;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBusTermsIterator;

extern void PySNLBusTerms_LinkPyType();

} /* PYNAJA namespace */
 
