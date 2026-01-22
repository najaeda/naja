// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "Python.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLDesignObject;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::SNLDesignObject* object_;
} PySNLDesignObject;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLDesignObject;

extern void         PySNLDesignObject_LinkPyType();

#define IsPySNLDesignObject(v) (PyObject_TypeCheck(v, &PyTypeSNLDesignObject))
#define PYSNLDesignObject(v)   ((PySNLDesignObject*)(v))
#define PYSNLDesignObject_O(v) (PYSNLDesignObject(v)->object_)

} /* PYNAJA namespace */
 
