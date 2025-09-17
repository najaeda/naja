// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_INST_TERM_OCCURRENCES_H_
#define __PY_SNL_INST_TERM_OCCURRENCES_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::NL {
  class SNLInstTermOccurrence;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLInstTermOccurrence>* object_;
} PySNLInstTermOccurrences;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLInstTermOccurrence>::Iterator* object_;
  PySNLInstTermOccurrences* container_;
} PySNLInstTermOccurrencesIterator;

extern PyTypeObject PyTypeSNLInstTermOccurrences;
extern PyTypeObject PyTypeSNLInstTermOccurrencesIterator;

extern void PySNLInstTermOccurrences_LinkPyType();

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_INST_TERM_OCCURRENCES_H_ */
