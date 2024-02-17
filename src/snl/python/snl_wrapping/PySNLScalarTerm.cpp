// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLScalarTerm.h"

#include "PyInterface.h"
#include "PySNLDesign.h"

#include "SNLScalarTerm.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)   GENERIC_METHOD_HEAD(Instance, instance, function)

static PyObject* PySNLScalarTerm_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  int intDir = 0;
  if (not PyArg_ParseTuple(args, "Oi|s:SNLDB.create", &arg0, &intDir, &arg1)) {
    setError("malformed SNLScalarTerm create method");
    return nullptr;
  }
  SNLName name;
  if (arg1) {
    name = SNLName(arg1);
  }

  SNLTerm::Direction direction = SNLNetComponent::Direction::DirectionEnum(intDir);

  SNLScalarTerm* term = nullptr;
  SNLTRY
  if (IsPySNLDesign(arg0)) {
    term = SNLScalarTerm::create(PYSNLDesign_O(arg0), direction, name);
  } else {
    setError("SNLScalarTerm create accepts SNLDesign as first argument");
    return nullptr;
  }
  SNLCATCH
  return PySNLScalarTerm_Link(term);
}

PyMethodDef PySNLScalarTerm_Methods[] = {
  { "create", (PyCFunction)PySNLScalarTerm_create, METH_VARARGS|METH_STATIC,
    "SNLScalarTerm creator"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLScalarTerm)

DBoLinkCreateMethod(SNLScalarTerm)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLScalarTerm)
PyTypeObjectDefinitions(SNLScalarTerm)

}
