// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_INSTTERM_H_
#define __PY_SNL_INSTTERM_H_

#include "PySNLNetComponent.h"
#include "SNLInstTerm.h"

namespace naja::SNL {
  class SNLInstTerm;
}

namespace PYSNL {

typedef struct {
  PySNLNetComponent parent_;
} PySNLInstTerm;

extern PyTypeObject PyTypeSNLInstTerm;

extern PyObject* PySNLInstTerm_Link(naja::SNL::SNLInstTerm*);
extern void PySNLInstTerm_LinkPyType();
extern void PySNLInstTerm_postModuleInit();

#define IsPySNLInstTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLInstTerm))
#define PYSNLInstTerm(v)   ((PySNLInstTerm*)(v))
#define PYSNLInstTerm_O(v) (static_cast<naja::SNL::SNLInstTerm*>(PYSNLInstTerm(v)->parent_.parent_.object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_INSTTERM_H_ */
