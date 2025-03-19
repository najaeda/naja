// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_NET_COMPONENTS_H_
#define __PY_SNL_NET_COMPONENTS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::NL {
  class SNLNetComponent;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLNetComponent*>* object_;
} PySNLNetComponents;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::SNLNetComponent*>::Iterator* object_;
  PySNLNetComponents* container_;
} PySNLNetComponentsIterator;

extern PyTypeObject PyTypeSNLNetComponents;
extern PyTypeObject PyTypeSNLNetComponentsIterator;

extern void PySNLNetComponents_LinkPyType();

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_NET_COMPONENTS_H_ */