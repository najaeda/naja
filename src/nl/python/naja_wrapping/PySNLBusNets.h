// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaPythonExport.h"
#include "NajaCollection.h"

namespace naja::NL {
  class SNLBusNet;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLBusNet*>* object_;
} PySNLBusNets;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLBusNet*>::Iterator* object_;
  PySNLBusNets* container_;
} PySNLBusNetsIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBusNets;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBusNetsIterator;

extern void PySNLBusNets_LinkPyType();

} /* PYNAJA namespace */
 
