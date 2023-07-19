// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_BITNETS_H_
#define __PY_SNL_BITNETS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLBitNet;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLBitNet*>* object_;
} PySNLBitNets;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLBitNet*>::Iterator* object_;
  PySNLBitNets* container_;
} PySNLBitNetsIterator;

extern PyTypeObject PyTypeSNLBitNets;
extern PyTypeObject PyTypeSNLBitNetsIterator;

extern void PySNLBitNets_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_BITNETS_H_ */
