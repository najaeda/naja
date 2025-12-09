// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_BITNETS_H_
#define __PY_SNL_BITNETS_H_

#include <Python.h>
#include "NajaPythonExport.h"
#include "NajaCollection.h"

namespace naja::NL {
  class SNLBitNet;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLBitNet*>* object_;
} PySNLBitNets;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLBitNet*>::Iterator* object_;
  PySNLBitNets* container_;
} PySNLBitNetsIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBitNets;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBitNetsIterator;

extern void PySNLBitNets_LinkPyType();

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_BITNETS_H_ */
