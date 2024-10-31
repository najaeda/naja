// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_INST_TERM_OCCURRENCES_H_
#define __PY_SNL_INST_TERM_OCCURRENCES_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLInstTermOccurrence;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLInstTermOccurrence>* object_;
} PySNLInstTermOccurrences;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLInstTermOccurrence>::Iterator* object_;
  PySNLInstTermOccurrences* container_;
} PySNLInstTermOccurrencesIterator;

extern PyTypeObject PyTypeSNLInstTermOccurrences;
extern PyTypeObject PyTypeSNLInstTermOccurrencesIterator;

extern void PySNLInstTermOccurrences_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_INST_TERM_OCCURRENCES_H_ */
