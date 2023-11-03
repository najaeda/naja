// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLUniverse.h"

#include "PyInterface.h"
#include "SNLUniverse.h"

namespace PYSNL {

using namespace naja::SNL;

static PyObject* PySNLUniverse_create() {
  SNLUniverse* universe = nullptr;
  SNLTRY
  universe = SNLUniverse::create();
  SNLCATCH
  return PySNLUniverse_Link(universe);
}

static PyObject* PySNLUniverse_get() {
  auto universe = SNLUniverse::get();
  return PySNLUniverse_Link(universe);
}

DBoDestroyAttribute(PySNLUniverse_destroy, PySNLUniverse)

PyMethodDef PySNLUniverse_Methods[] = {
  { "create", (PyCFunction)PySNLUniverse_create, METH_NOARGS|METH_STATIC,
    "create the SNL Universe (static object)"},
  { "destroy", (PyCFunction)PySNLUniverse_destroy, METH_NOARGS,
    "destroy the associated SNLUniverse"},
  { "get", (PyCFunction)PySNLUniverse_get, METH_NOARGS|METH_STATIC,
    "get the SNL Universe (static object)"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLUniverse)

DBoLinkCreateMethod(SNLUniverse)
PyTypeSNLObjectWithoutSNLIDLinkPyType(SNLUniverse)
PyTypeObjectDefinitions(SNLUniverse)

}
