// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaCollection.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLTerm;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLTerm*>* object_;
} PySNLTerms;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLTerm*>::Iterator* object_;
  PySNLTerms* container_;
} PySNLTermsIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLTerms;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLTermsIterator;

extern void PySNLTerms_LinkPyType();

} /* PYNAJA namespace */
 
