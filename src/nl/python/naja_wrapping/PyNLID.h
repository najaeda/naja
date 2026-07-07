// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <Python.h>

#include "NLID.h"
#include "NajaPythonExport.h"

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::NLID* object_;
} PyNLID;

NAJA_PY_EXPORT extern PyTypeObject PyTypeNLID;

extern PyObject* PyNLID_Link(const naja::NL::NLID& id);
extern void PyNLID_LinkPyType();
extern void PyNLID_postModuleInit();

#define IsPyNLID(v) (PyObject_TypeCheck(v, &PyTypeNLID))
#define PYNLID(v)   ((PyNLID*)(v))
#define PYNLID_O(v) (*PYNLID(v)->object_)

} // namespace PYNAJA
