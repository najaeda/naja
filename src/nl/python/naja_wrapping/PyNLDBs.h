// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_NL_DBS_H_
#define __PY_NL_DBS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class NLDB;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::NLDB*>* object_;
} PyNLDBs;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::NLDB*>::Iterator* object_;
  PyNLDBs* container_;
} PyNLDBsIterator;

extern PyTypeObject PyTypeNLDBs;
extern PyTypeObject PyTypeNLDBsIterator;

extern void PyNLDBs_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_NL_DBS_H_ */
