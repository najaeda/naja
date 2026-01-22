// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaCollection.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLScalarTerm;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLScalarTerm*>* object_;
} PySNLScalarTerms;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLScalarTerm*>::Iterator* object_;
  PySNLScalarTerms* container_;
} PySNLScalarTermsIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLScalarTerms;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLScalarTermsIterator;

extern void PySNLScalarTerms_LinkPyType();

} /* PYNAJA namespace */
 
