// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <Python.h>
#include "NajaCollection.h"

namespace naja::NL {
  class SNLOccurrence;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLOccurrence>* object_;
} PySNLOccurrences;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLOccurrence>::Iterator* object_;
  PySNLOccurrences* container_;
} PySNLOccurrencesIterator;

extern PyTypeObject PyTypeSNLOccurrences;
extern PyTypeObject PyTypeSNLOccurrencesIterator;

extern void PySNLOccurrences_LinkPyType();

} /* PYNAJA namespace */