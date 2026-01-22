// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaCollection.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class NLDB;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::NLDB*>* object_;
} PyNLDBs;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::NLDB*>::Iterator* object_;
  PyNLDBs* container_;
} PyNLDBsIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeNLDBs;
NAJA_PY_EXPORT extern PyTypeObject PyTypeNLDBsIterator;

extern void PyNLDBs_LinkPyType();

} /* PYNAJA namespace */
 
