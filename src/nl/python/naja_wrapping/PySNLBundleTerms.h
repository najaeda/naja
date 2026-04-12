// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <Python.h>
#include "NajaPythonExport.h"
#include "NajaCollection.h"

namespace naja::NL {
  class SNLBundleTerm;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLBundleTerm*>* object_;
} PySNLBundleTerms;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLBundleTerm*>::Iterator* object_;
  PySNLBundleTerms* container_;
} PySNLBundleTermsIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBundleTerms;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBundleTermsIterator;

extern void PySNLBundleTerms_LinkPyType();

} /* PYNAJA namespace */

