// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_NETS_H_
#define __PY_SNL_NETS_H_

#include <Python.h>
#include "NajaCollection.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLNet;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLNet*>* object_;
} PySNLNets;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLNet*>::Iterator* object_;
  PySNLNets* container_;
} PySNLNetsIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLNets;
NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLNetsIterator;

extern void PySNLNets_LinkPyType();

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_NETS_H_ */