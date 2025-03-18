// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_NET_COMPONENTS_H_
#define __PY_SNL_NET_COMPONENTS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLNetComponent;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLNetComponent*>* object_;
} PySNLNetComponents;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLNetComponent*>::Iterator* object_;
  PySNLNetComponents* container_;
} PySNLNetComponentsIterator;

extern PyTypeObject PyTypeSNLNetComponents;
extern PyTypeObject PyTypeSNLNetComponentsIterator;

extern void PySNLNetComponents_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_NET_COMPONENTS_H_ */