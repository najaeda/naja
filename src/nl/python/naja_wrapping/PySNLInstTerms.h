// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_INST_TERMS_H_
#define __PY_SNL_INST_TERMS_H_

#include <Python.h>
#include "NajaCollection.h"

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

extern PyTypeObject PyTypeSNLInstTerms;
extern PyTypeObject PyTypeSNLInstTermsIterator;

extern void PySNLInstTerms_LinkPyType();

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_INST_TERMS_H_ */
