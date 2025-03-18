// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_INSTANCES_H_
#define __PY_SNL_INSTANCES_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLInstance;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLInstance*>* object_;
} PySNLInstances;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLInstance*>::Iterator* object_;
  PySNLInstances* container_;
} PySNLInstancesIterator;

extern PyTypeObject PyTypeSNLInstances;
extern PyTypeObject PyTypeSNLInstancesIterator;

extern void PySNLInstances_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_INSTANCES_H_ */
