// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_DESIGN_OBJECT_H_
#define __PY_SNL_DESIGN_OBJECT_H_

#include "Python.h"
#include "SNLDesignObject.h"

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::SNLDesignObject* object_;
} PySNLDesignObject;

extern PyTypeObject PyTypeSNLDesignObject;

extern void         PySNLDesignObject_LinkPyType();

#define IsPySNLDesignObject(v) (PyObject_TypeCheck(v, &PyTypeSNLDesignObject))
#define PYSNLDesignObject(v)   ((PySNLDesignObject*)(v))
#define PYSNLDesignObject_O(v) (PYSNLDesignObject(v)->object_)

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_DESIGN_OBJECT_H_ */