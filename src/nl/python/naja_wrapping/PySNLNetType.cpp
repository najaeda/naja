// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLNetType.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeObjectDefinitions(SNLNetType)

extern void PySNLNetType_postModuleInit() {
  PyObject* constant;
  LoadObjectConstant(PyTypeSNLNetType.tp_dict, SNLNet::Type::Standard,  "Standard");
  LoadObjectConstant(PyTypeSNLNetType.tp_dict, SNLNet::Type::Supply0,   "Supply0");
  LoadObjectConstant(PyTypeSNLNetType.tp_dict, SNLNet::Type::Supply1,   "Supply1");
  LoadObjectConstant(PyTypeSNLNetType.tp_dict, SNLNet::Type::Assign0,   "Assign0");
  LoadObjectConstant(PyTypeSNLNetType.tp_dict, SNLNet::Type::Assign1,   "Assign1");
}

PyMethodDef PySNLNetType_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

extern void PySNLNetType_LinkPyType() {
  PyTypeSNLNetType.tp_methods = PySNLNetType_Methods;
}

}