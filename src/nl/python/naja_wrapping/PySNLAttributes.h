// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaPythonExport.h"
#include "NajaCollection.h"

namespace naja::NL {
  class SNLAttribute;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLAttribute>* object_;
} PySNLAttributes;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLAttribute>::Iterator* object_;
  PySNLAttributes* container_;
} PySNLAttributesIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLAttributes;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLAttributesIterator;

extern void PySNLAttributes_LinkPyType();

} /* PYNAJA namespace */
 
