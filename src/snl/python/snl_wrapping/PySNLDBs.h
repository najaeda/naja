// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_DBS_H_
#define __PY_SNL_DBS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLDB;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLDB*>* object_;
} PySNLDBs;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLDB*>::Iterator* object_;
  PySNLDBs* container_;
} PySNLDBsIterator;

extern PyTypeObject PyTypeSNLDBs;
extern PyTypeObject PyTypeSNLDBsIterator;

extern void PySNLDBs_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_DBS_H_ */
