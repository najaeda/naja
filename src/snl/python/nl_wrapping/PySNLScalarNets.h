// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_SCALAR_NETS_H_
#define __PY_SNL_SCALAR_NETS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLScalarNet;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLScalarNet*>* object_;
} PySNLScalarNets;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLScalarNet*>::Iterator* object_;
  PySNLScalarNets* container_;
} PySNLScalarNetsIterator;

extern PyTypeObject PyTypeSNLScalarNets;
extern PyTypeObject PyTypeSNLScalarNetsIterator;

extern void PySNLScalarNets_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_SCALAR_NETS_H_ */
