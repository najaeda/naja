// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLTermDirection.h"

namespace PYSNL {

using namespace naja::SNL;

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
  //`PyTypeSNLTermDirection.tp_dealloc     = (destructor) PyNetDirection_DeAlloc;
  //PyTypeSNLTermDirection.tp_richcompare = (richcmpfunc)PyNetDirection_Cmp;
  //PyTypeSNLTermDirection.tp_repr        = (reprfunc)   PyNetDirection_Repr;
  //PyTypeSNLTermDirection.tp_str         = (reprfunc)   PyNetDirection_Str;
  //PyTypeSNLTermDirection.tp_hash        = (hashfunc)   PyNetDirection_Hash;
  PyTypeSNLTermDirection.tp_methods = PySNLTermDirection_Methods;
}

}