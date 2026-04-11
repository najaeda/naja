// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "NajaPythonExport.h"
#include "PySNLTerm.h"
#include "SNLBundleTerm.h"

namespace PYNAJA {

typedef struct {
  PySNLTerm parent_;
} PySNLBundleTerm;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLBundleTerm;

extern PyObject*    PySNLBundleTerm_Link(naja::NL::SNLBundleTerm* t);
extern void         PySNLBundleTerm_LinkPyType();

#define IsPySNLBundleTerm(v) (PyObject_TypeCheck(v, &PyTypeSNLBundleTerm))
#define PYSNLBundleTerm(v)   ((PySNLBundleTerm*)(v))
#define PYSNLBundleTerm_O(v) (static_cast<naja::NL::SNLBundleTerm*>(PYSNLBundleTerm(v)->parent_.parent_.parent_.object_))

} // namespace PYNAJA
