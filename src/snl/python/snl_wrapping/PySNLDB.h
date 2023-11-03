// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_DB_H_
#define __PY_SNL_DB_H_

#include <Python.h>
#include "SNLDB.h"

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLDB* object_;
} PySNLDB;

extern PyTypeObject PyTypeSNLDB;

extern PyObject*    PySNLDB_Link(naja::SNL::SNLDB* u);
extern void         PySNLDB_LinkPyType();

#define IsPySNLDB(v) (PyObject_TypeCheck(v, &PyTypeSNLDB))
#define PYSNLDB(v)   ((PySNLDB*)(v))
#define PYSNLDB_O(v) (PYSNLDB(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_DB_H_ */
