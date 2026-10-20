// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaCollection.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class NLClock;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::NLClock*>* object_;
} PyNLClocks;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::NLClock*>::Iterator* object_;
  PyNLClocks* container_;
} PyNLClocksIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeNLClocks;
NAJA_PY_EXPORT extern PyTypeObject PyTypeNLClocksIterator;

extern void PyNLClocks_LinkPyType();

} /* PYNAJA namespace */
