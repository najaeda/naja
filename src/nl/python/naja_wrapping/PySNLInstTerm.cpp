// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstTerm.h"

#include "SNLInstTerm.h"
#include "SNLDesignModeling.h"

#include "PyInterface.h"
#include "PySNLBitTerm.h"
#include "PySNLInstance.h"

namespace PYNAJA {

using namespace naja::NL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(SNLInstTerm, function)

DBoLinkCreateMethod(SNLInstTerm)
PyTypeInheritedObjectDefinitions(SNLInstTerm, SNLNetComponent)

GetObjectMethod(SNLInstTerm, SNLInstance, getInstance)
GetObjectMethod(SNLInstTerm, SNLBitTerm, getBitTerm)

static PyObject* PySNLInstTerm_getRole(PySNLInstTerm* self) {
  METHOD_HEAD("SNLInstTerm.getRole()")
  return PyLong_FromLong(static_cast<long>(SNLDesignModeling::getTermRole(selfObject)));
}

static PyObject* PySNLInstTerm_getResetActiveLevel(PySNLInstTerm* self) {
  METHOD_HEAD("SNLInstTerm.getResetActiveLevel()")
  return PyLong_FromLong(static_cast<long>(SNLDesignModeling::getResetActiveLevel(selfObject)));
}

#define INST_TERM_ROLE_PREDICATE(PYNAME, CPPNAME)                       \
  static PyObject* PySNLInstTerm_##PYNAME(PySNLInstTerm* self) {        \
    METHOD_HEAD("SNLInstTerm." #PYNAME "()")                            \
    if (SNLDesignModeling::CPPNAME(selfObject)) Py_RETURN_TRUE;          \
    Py_RETURN_FALSE;                                                     \
  }

INST_TERM_ROLE_PREDICATE(is_clock, isClock)
INST_TERM_ROLE_PREDICATE(is_async_reset, isAsyncReset)
INST_TERM_ROLE_PREDICATE(is_async_set, isAsyncSet)
INST_TERM_ROLE_PREDICATE(is_reset, isReset)
INST_TERM_ROLE_PREDICATE(is_enable, isEnable)
INST_TERM_ROLE_PREDICATE(is_data_input, isDataInput)
INST_TERM_ROLE_PREDICATE(is_data_output, isDataOutput)

static PyObject* PySNLInstTerm_is_data(PySNLInstTerm* self) {
  METHOD_HEAD("SNLInstTerm.is_data()")
  if (SNLDesignModeling::isDataInput(selfObject) ||
      SNLDesignModeling::isDataOutput(selfObject)) Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

#undef INST_TERM_ROLE_PREDICATE

PyMethodDef PySNLInstTerm_Methods[] = {
  { "getBitTerm", (PyCFunction)PySNLInstTerm_getBitTerm, METH_NOARGS,
    "get the SNLBitTerm represented by this SNLInstTerm."},
  { "getInstance", (PyCFunction)PySNLInstTerm_getInstance, METH_NOARGS,
    "get the SNLInstance containing this SNLInstTerm."},
  {"getRole", (PyCFunction)PySNLInstTerm_getRole, METH_NOARGS, "get the primitive term role."},
  {"getResetActiveLevel", (PyCFunction)PySNLInstTerm_getResetActiveLevel, METH_NOARGS, "get reset/set active level."},
  {"is_clock", (PyCFunction)PySNLInstTerm_is_clock, METH_NOARGS, "whether this term is a clock."},
  {"is_async_reset", (PyCFunction)PySNLInstTerm_is_async_reset, METH_NOARGS, "whether this term is an asynchronous reset."},
  {"is_async_set", (PyCFunction)PySNLInstTerm_is_async_set, METH_NOARGS, "whether this term is an asynchronous set."},
  {"is_reset", (PyCFunction)PySNLInstTerm_is_reset, METH_NOARGS, "whether this term is a reset."},
  {"is_enable", (PyCFunction)PySNLInstTerm_is_enable, METH_NOARGS, "whether this term is an enable."},
  {"is_data", (PyCFunction)PySNLInstTerm_is_data, METH_NOARGS, "whether this term carries data."},
  {"is_data_input", (PyCFunction)PySNLInstTerm_is_data_input, METH_NOARGS, "whether this term is a data input."},
  {"is_data_output", (PyCFunction)PySNLInstTerm_is_data_output, METH_NOARGS, "whether this term is a data output."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLInstTerm)

PyTypeNLFinalObjectWithNLIDLinkPyType(SNLInstTerm)

}
