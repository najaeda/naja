// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_PARAMETER_H_
#define __PY_SNL_PARAMETER_H_

#include <Python.h>

namespace naja::SNL {
  class SNLParameter;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::SNL::SNLParameter* object_;
} PySNLParameter;

extern PyTypeObject PyTypeSNLParameter;

extern PyObject* PySNLParameter_Link(naja::SNL::SNLParameter* u);
extern void PySNLParameter_LinkPyType();
extern void PySNLParameter_postModuleInit();

#define IsPySNLParameter(v) (PyObject_TypeCheck(v, &PyTypeSNLParameter))
#define PYSNLParameter(v)   ((PySNLParameter*)(v))
#define PYSNLParameter_O(v) (PYSNLParameter(v)->object_)

} /* PYSNL namespace */
 
#endif /* __PY_SNL_PARAMETER_H_ */
