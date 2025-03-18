// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_NL_DB_H_
#define __PY_NL_DB_H_

#include <Python.h>
#include "NLDB.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::NLDB* object_;
} PyNLDB;

extern PyTypeObject PyTypeNLDB;

extern PyObject*    PyNLDB_Link(naja::SNL::NLDB* u);
extern void         PyNLDB_LinkPyType();

#define IsPyNLDB(v) (PyObject_TypeCheck(v, &PyTypeNLDB))
#define PYNLDB(v)   ((PyNLDB*)(v))
#define PYNLDB_O(v) (PYNLDB(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_DB_H_ */