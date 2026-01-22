// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaPythonExport.h"
#include "NajaCollection.h"

namespace naja::NL {
  class SNLBitTerm;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLBitTerm*>* object_;
} PySNLBitTerms;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLBitTerm*>::Iterator* object_;
  PySNLBitTerms* container_;
} PySNLBitTermsIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBitTerms;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBitTermsIterator;

extern void PySNLBitTerms_LinkPyType();

} /* PYNAJA namespace */
 
