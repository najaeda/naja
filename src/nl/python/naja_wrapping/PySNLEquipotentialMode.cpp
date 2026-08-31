// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLEquipotentialMode.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeObjectDefinitions(SNLEquipotentialMode)

void PySNLEquipotentialMode_postModuleInit() {
  PyObject* constant;
  LoadObjectConstant(
    PyTypeSNLEquipotentialMode.tp_dict,
    SNLEquipotential::Mode::Standard,
    "Standard");
  LoadObjectConstant(
    PyTypeSNLEquipotentialMode.tp_dict,
    SNLEquipotential::Mode::TraverseAssigns,
    "TraverseAssigns");
}

PyMethodDef PySNLEquipotentialMode_Methods[] = {
  {nullptr, nullptr, 0, nullptr}
};

void PySNLEquipotentialMode_LinkPyType() {
  PyTypeSNLEquipotentialMode.tp_methods = PySNLEquipotentialMode_Methods;
}

}  // namespace PYNAJA
