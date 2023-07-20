// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_BIT_TERMS_H_
#define __PY_SNL_BIT_TERMS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLBitTerm;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLBitTerm*>* object_;
} PySNLBitTerms;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLBitTerm*>::Iterator* object_;
  PySNLBitTerms* container_;
} PySNLBitTermsIterator;

extern PyTypeObject PyTypeSNLBitTerms;
extern PyTypeObject PyTypeSNLBitTermsIterator;

extern void PySNLBitTerms_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_BIT_TERMS_H_ */
