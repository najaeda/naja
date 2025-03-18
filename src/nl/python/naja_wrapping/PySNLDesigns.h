// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_DESIGNS_H_
#define __PY_SNL_DESIGNS_H_

#include <Python.h>
#include "NajaCollection.h"

namespace naja::SNL {
  class SNLDesign;
}

namespace PYSNL {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLDesign*>* object_;
} PySNLDesigns;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::SNL::SNLDesign*>::Iterator* object_;
  PySNLDesigns* container_;
} PySNLDesignsIterator;

extern PyTypeObject PyTypeSNLDesigns;
extern PyTypeObject PyTypeSNLDesignsIterator;

extern void PySNLDesigns_LinkPyType();

} /* PYSNL namespace */
 
#endif /* __PY_SNL_DESIGNS_H_ */
