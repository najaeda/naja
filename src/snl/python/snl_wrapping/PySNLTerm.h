// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_TERM_H_
#define __PY_SNL_TERM_H_

#include "PySNLNetComponent.h"
#include "SNLTerm.h"

namespace PYSNL {

typedef struct {
  PySNLNetComponent parent_;
} PySNLTerm;

extern PyTypeObject PyTypeSNLTerm;

extern PyObject* PySNLTerm_Link(naja::SNL::SNLTerm*);
extern void PySNLTerm_LinkPyType();
extern void PySNLTerm_postModuleInit();

#define IsPySNLTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLTerm))
#define PYSNLTerm(v)   (static_cast<PySNLTerm*>(v))
#define PYSNLTerm_O(v) (static_cast<naja::SNL::SNLTerm*>(PYSNLTerm(v)->parent_->parent_->object_))

} /* PYSNL namespace */
 
#endif /* __PY_SNL_TERM_H_ */
