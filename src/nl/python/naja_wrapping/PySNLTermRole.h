// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NajaPythonExport.h"
#include "PyInterface.h"
#include "SNLDesignModeling.h"

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::SNLDesignModeling::SNLTermRole* object_;
} PySNLTermRole;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLTermRole;
extern void PySNLTermRole_LinkPyType();
extern void PySNLTermRole_postModuleInit();

}
