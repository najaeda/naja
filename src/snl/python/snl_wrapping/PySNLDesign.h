// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_DESIGN_H_
#define __PY_SNL_DESIGN_H_

#include <Python.h>

namespace naja::SNL {
  class SNLDesign;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLDesign* object_;
} PySNLDesign;

extern PyTypeObject PyTypeSNLDesign;

extern PyObject*    PySNLDesign_Link(naja::SNL::SNLDesign* u);
extern void         PySNLDesign_LinkPyType();

#define IsPySNLDesign(v) (PyObject_TypeCheck(v, &PyTypeSNLDesign))
#define PYSNLDesign(v)   ((PySNLDesign*)(v))
#define PYSNLDesign_O(v) (PYSNLDesign(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_DESIGN_H_ */
