// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLTermDirection.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeObjectDefinitions(SNLTermDirection)

extern void PySNLTermDirection_postModuleInit() {
  PyObject* constant;
  LoadObjectConstant(PyTypeSNLTermDirection.tp_dict, SNLTerm::Direction::Input,   "Input");
  LoadObjectConstant(PyTypeSNLTermDirection.tp_dict, SNLTerm::Direction::Output,  "Output");
  LoadObjectConstant(PyTypeSNLTermDirection.tp_dict, SNLTerm::Direction::InOut,   "InOut");
}

PyMethodDef PySNLTermDirection_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

extern void PySNLTermDirection_LinkPyType() {
  PyTypeSNLTermDirection.tp_methods = PySNLTermDirection_Methods;
}

}