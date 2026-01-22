// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaPythonExport.h"

namespace naja::NL {
  class NLDB;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::NLDB* object_;
} PyNLDB;

NAJA_PY_EXPORT extern PyTypeObject PyTypeNLDB;

extern PyObject*    PyNLDB_Link(naja::NL::NLDB* u);
extern void         PyNLDB_LinkPyType();

#define IsPyNLDB(v) (PyObject_TypeCheck(v, &PyTypeNLDB))
#define PYNLDB(v)   ((PyNLDB*)(v))
#define PYNLDB_O(v) (PYNLDB(v)->object_)

} /* PYNAJA namespace */
 
