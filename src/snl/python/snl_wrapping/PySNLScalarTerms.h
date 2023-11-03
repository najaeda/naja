// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_SCALAR_TERMS_H_
#define __PY_SNL_SCALAR_TERMS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLScalarTerm;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLScalarTerm*>* object_;
} PySNLScalarTerms;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLScalarTerm*>::Iterator* object_;
  PySNLScalarTerms* container_;
} PySNLScalarTermsIterator;

extern PyTypeObject PyTypeSNLScalarTerms;
extern PyTypeObject PyTypeSNLScalarTermsIterator;

extern void PySNLScalarTerms_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_SCALAR_TERMS_H_ */
