// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_DESIGN_H_
#define __PY_SNL_DESIGN_H_

#include <Python.h>
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLDesign;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::SNLDesign* object_;
} PySNLDesign;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLDesign;

extern PyObject*    PySNLDesign_Link(naja::NL::SNLDesign* u);
extern void         PySNLDesign_LinkPyType();

#define IsPySNLDesign(v) (PyObject_TypeCheck(v, &PyTypeSNLDesign))
#define PYSNLDesign(v)   ((PySNLDesign*)(v))
#define PYSNLDesign_O(v) (PYSNLDesign(v)->object_)

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_DESIGN_H_ */