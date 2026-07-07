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
} PyLogicCone;

NAJA_PY_EXPORT extern PyTypeObject PyTypeLogicCone;

extern PyObject* PyLogicCone_Link(
  const naja::NAJA_METRICS::LogicCone& logicalCone);
extern void PyLogicCone_LinkPyType();
extern void PyLogicCone_postModuleInit();

#define IsPyLogicCone(v) (PyObject_TypeCheck(v, &PyTypeLogicCone))
#define PYLogicCone(v) ((PyLogicCone*)(v))
#define PYLogicCone_O(v) (PYLogicCone(v)->object_)

}  // namespace PYNAJA
