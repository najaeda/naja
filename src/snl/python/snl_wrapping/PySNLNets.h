// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_NETS_H_
#define __PY_SNL_NETS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLNet;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLNet*>* object_;
} PySNLNets;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLNet*>::Iterator* object_;
  PySNLNets* container_;
} PySNLNetsIterator;

extern PyTypeObject PyTypeSNLNets;
extern PyTypeObject PyTypeSNLNetsIterator;

extern void PySNLNets_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_NETS_H_ */
