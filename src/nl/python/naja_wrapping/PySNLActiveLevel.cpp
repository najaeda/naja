// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
#include "PySNLActiveLevel.h"

namespace PYNAJA {
using namespace naja::NL;

PyTypeObjectDefinitions(SNLActiveLevel)

void PySNLActiveLevel_postModuleInit() {
  PyObject* constant;
  using Level = SNLDesignModeling::SNLActiveLevel;
  LoadObjectConstant(PyTypeSNLActiveLevel.tp_dict, Level::High, "High");
  LoadObjectConstant(PyTypeSNLActiveLevel.tp_dict, Level::Low, "Low");
  LoadObjectConstant(PyTypeSNLActiveLevel.tp_dict, Level::NA, "NA");
}

PyMethodDef PySNLActiveLevel_Methods[] = {{nullptr, nullptr, 0, nullptr}};
void PySNLActiveLevel_LinkPyType() {
  PyTypeSNLActiveLevel.tp_methods = PySNLActiveLevel_Methods;
}
}
