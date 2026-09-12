// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PySNLDesign.h"

namespace PYNAJA {

PyObject* PySNLDesign_setSequentialModel(
    PySNLDesign* self, PyObject* args, PyObject* kwargs);
PyObject* PySNLDesign_hasSequentialModel(PySNLDesign* self);

}  // namespace PYNAJA
