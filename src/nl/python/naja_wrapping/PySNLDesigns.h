// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaCollection.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLDesign;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLDesign*>* object_;
} PySNLDesigns;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLDesign*>::Iterator* object_;
  PySNLDesigns* container_;
} PySNLDesignsIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLDesigns;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLDesignsIterator;

extern void PySNLDesigns_LinkPyType();

} /* PYNAJA namespace */
 
