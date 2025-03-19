// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_BUS_NETS_H_
#define __PY_SNL_BUS_NETS_H_

#include <Python.h>
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

extern PyTypeObject PyTypeSNLBusNets;
extern PyTypeObject PyTypeSNLBusNetsIterator;

extern void PySNLBusNets_LinkPyType();

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_BUS_NETS_H_ */
