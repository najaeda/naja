// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaCollection.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLInstTerm;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLInstTerm*>* object_;
} PySNLInstTerms;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLInstTerm*>::Iterator* object_;
  PySNLInstTerms* container_;
} PySNLInstTermsIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLInstTerms;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLInstTermsIterator;

extern void PySNLInstTerms_LinkPyType();

} /* PYNAJA namespace */
 
