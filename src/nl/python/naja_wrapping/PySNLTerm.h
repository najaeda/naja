// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_TERM_H_
#define __PY_SNL_TERM_H_

#include "PySNLNetComponent.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class SNLTerm;
}

namespace PYNAJA {

typedef struct {
  PySNLNetComponent parent_;
} PySNLTerm;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLTerm;

extern PyObject* PySNLTerm_Link(naja::NL::SNLTerm*);
extern void PySNLTerm_LinkPyType();
extern void PySNLTerm_postModuleInit();

#define IsPySNLTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLTerm))
#define PYSNLTerm(v)   (static_cast<PySNLTerm*>(v))
#define PYSNLTerm_O(v) (static_cast<naja::NL::SNLTerm*>(PYSNLTerm(v)->parent_->parent_->object_))

} /* PYNAJA namespace */
 
#endif /* __PY_SNL_TERM_H_ */
