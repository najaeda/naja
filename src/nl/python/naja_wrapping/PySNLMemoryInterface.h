// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PySNLDesign.h"
#include "PySNLInstance.h"

namespace PYNAJA {

PyObject* PySNLDesign_hasMemoryInterface(PySNLDesign* self);
PyObject* PySNLDesign_getMemoryInterface(PySNLDesign* self);
PyObject* PySNLInstance_getMemoryInterface(PySNLInstance* self);

}  // namespace PYNAJA
