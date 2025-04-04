// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PY_SNL_EQUIPOTENTIAL_H_
#define __PY_SNL_EQUIPOTENTIAL_H_

#include <Python.h>

namespace naja::NL {
  class SNLEquipotential;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::SNLEquipotential* object_;
} PySNLEquipotential;

extern PyTypeObject PyTypeSNLEquipotential;

extern PyObject*  PySNLEquipotential_Link(naja::NL::SNLEquipotential* equipotential);
extern void       PySNLEquipotential_LinkPyType();

#define IsPySNLEquipotential(v) (PyObject_TypeCheck(v, &PyTypeSNLEquipotential))
#define PYSNLEquipotential(v)   ((PySNLEquipotential*)(v))
#define PYSNLEquipotential_O(v) (PYSNLEquipotential(v)->object_)

} // PYNAJA namespace
 
#endif // __PY_SNL_EQUIPOTENTIAL_H_