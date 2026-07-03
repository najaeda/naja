// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <Python.h>

#include "NajaPythonExport.h"

namespace naja::NAJA_METRICS {
  class LogicCone;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NAJA_METRICS::LogicCone* object_;
} PySNLLogicalCone;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLLogicalCone;

extern PyObject* PySNLLogicalCone_Link(
  const naja::NAJA_METRICS::LogicCone& logicalCone);
extern void PySNLLogicalCone_LinkPyType();
extern void PySNLLogicalCone_postModuleInit();

#define IsPySNLLogicalCone(v) (PyObject_TypeCheck(v, &PyTypeSNLLogicalCone))
#define PYSNLLogicalCone(v) ((PySNLLogicalCone*)(v))
#define PYSNLLogicalCone_O(v) (PYSNLLogicalCone(v)->object_)

}  // namespace PYNAJA
