// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_TERMS_H_
#define __PY_SNL_TERMS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLTerm;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLTerm*>* object_;
} PySNLTerms;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLTerm*>::Iterator* object_;
  PySNLTerms* container_;
} PySNLTermsIterator;

extern PyTypeObject PyTypeSNLTerms;
extern PyTypeObject PyTypeSNLTermsIterator;

extern void PySNLTerms_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_TERMS_H_ */
