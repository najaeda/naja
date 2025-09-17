// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_INSTTERM_H_
#define __PY_SNL_INSTTERM_H_

#include "PySNLNetComponent.h"
#include "SNLInstTerm.h"

namespace PYNAJA {

typedef struct {
  PySNLNetComponent parent_;
} PySNLInstTerm;

extern PyTypeObject PyTypeSNLInstTerm;

extern PyObject* PySNLInstTerm_Link(naja::NL::SNLInstTerm*);
extern void PySNLInstTerm_LinkPyType();
extern void PySNLInstTerm_postModuleInit();

#define IsPySNLInstTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLInstTerm))
#define PYSNLInstTerm(v)   ((PySNLInstTerm*)(v))
#define PYSNLInstTerm_O(v) (static_cast<naja::NL::SNLInstTerm*>(PYSNLInstTerm(v)->parent_.parent_.object_))

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_INSTTERM_H_ */
