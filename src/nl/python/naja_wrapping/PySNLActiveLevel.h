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
  naja::NL::SNLDesignModeling::SNLActiveLevel* object_;
} PySNLActiveLevel;

NAJA_PY_EXPORT extern PyTypeObject PyTypeSNLActiveLevel;
extern void PySNLActiveLevel_LinkPyType();
extern void PySNLActiveLevel_postModuleInit();

}
