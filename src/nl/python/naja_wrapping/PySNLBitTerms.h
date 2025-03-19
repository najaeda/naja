// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_BIT_TERMS_H_
#define __PY_SNL_BIT_TERMS_H_

#include <Python.h>
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

extern PyTypeObject PyTypeSNLBitTerms;
extern PyTypeObject PyTypeSNLBitTermsIterator;

extern void PySNLBitTerms_LinkPyType();

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_BIT_TERMS_H_ */