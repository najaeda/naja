// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_BUS_TERMS_H_
#define __PY_SNL_BUS_TERMS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLBusTerm;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLBusTerm*>* object_;
} PySNLBusTerms;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLBusTerm*>::Iterator* object_;
  PySNLBusTerms* container_;
} PySNLBusTermsIterator;

extern PyTypeObject PyTypeSNLBusTerms;
extern PyTypeObject PyTypeSNLBusTermsIterator;

extern void PySNLBusTerms_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_BUS_TERMS_H_ */
