// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_INST_PARAMETER_H_
#define __PY_SNL_INST_PARAMETER_H_

#include <Python.h>

namespace naja::SNL {
  class SNLInstParameter;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLInstParameter* object_;
} PySNLInstParameter;

extern PyTypeObject PyTypeSNLInstParameter;

extern PyObject* PySNLInstParameter_Link(naja::SNL::SNLInstParameter* u);
extern void PySNLInstParameter_LinkPyType();
extern void PySNLInstParameter_postModuleInit();

#define IsPySNLInstParameter(v) (PyObject_TypeCheck(v, &PyTypeSNLInstParameter))
#define PYSNLInstParameter(v)   ((PySNLInstParameter*)(v))
#define PYSNLInstParameter_O(v) (PYSNLInstParameter(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_INST_PARAMETER_H_ */
