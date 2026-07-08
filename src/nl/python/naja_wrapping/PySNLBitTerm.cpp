// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBitTerm.h"

#include "SNLScalarTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesignModeling.h"

#include "PyInterface.h"
#include "PySNLScalarTerm.h"
#include "PySNLBusTermBit.h"

namespace PYNAJA {

using namespace naja::NL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(SNLBitTerm, function)

static PyObject* PySNLBitTerm_getRole(PySNLBitTerm* self) {
  METHOD_HEAD("SNLBitTerm.getRole()")
  return PyLong_FromLong(static_cast<long>(SNLDesignModeling::getTermRole(selfObject)));
}

static PyObject* PySNLBitTerm_getResetActiveLevel(PySNLBitTerm* self) {
  METHOD_HEAD("SNLBitTerm.getResetActiveLevel()")
  return PyLong_FromLong(static_cast<long>(SNLDesignModeling::getResetActiveLevel(selfObject)));
}

#define TERM_ROLE_PREDICATE(PYNAME, CPPNAME)                        \
  static PyObject* PySNLBitTerm_##PYNAME(PySNLBitTerm* self) {      \
    METHOD_HEAD("SNLBitTerm." #PYNAME "()")                         \
    if (SNLDesignModeling::CPPNAME(selfObject)) Py_RETURN_TRUE;      \
    Py_RETURN_FALSE;                                                 \
  }

TERM_ROLE_PREDICATE(is_clock, isClock)
TERM_ROLE_PREDICATE(is_async_reset, isAsyncReset)
TERM_ROLE_PREDICATE(is_async_set, isAsyncSet)
TERM_ROLE_PREDICATE(is_sync_reset, isSyncReset)
TERM_ROLE_PREDICATE(is_sync_set, isSyncSet)
TERM_ROLE_PREDICATE(is_reset, isReset)
TERM_ROLE_PREDICATE(is_enable, isEnable)
TERM_ROLE_PREDICATE(is_data_input, isDataInput)
TERM_ROLE_PREDICATE(is_data_output, isDataOutput)

static PyObject* PySNLBitTerm_is_data(PySNLBitTerm* self) {
  METHOD_HEAD("SNLBitTerm.is_data()")
  if (SNLDesignModeling::isDataInput(selfObject) ||
      SNLDesignModeling::isDataOutput(selfObject)) Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

#undef TERM_ROLE_PREDICATE

PyMethodDef PySNLBitTerm_Methods[] = {
  {"getRole", (PyCFunction)PySNLBitTerm_getRole, METH_NOARGS, "get the primitive term role."},
  {"getResetActiveLevel", (PyCFunction)PySNLBitTerm_getResetActiveLevel, METH_NOARGS, "get reset/set active level."},
  {"is_clock", (PyCFunction)PySNLBitTerm_is_clock, METH_NOARGS, "whether this term is a clock."},
  {"is_async_reset", (PyCFunction)PySNLBitTerm_is_async_reset, METH_NOARGS, "whether this term is an asynchronous reset."},
  {"is_async_set", (PyCFunction)PySNLBitTerm_is_async_set, METH_NOARGS, "whether this term is an asynchronous set."},
  {"is_sync_reset", (PyCFunction)PySNLBitTerm_is_sync_reset, METH_NOARGS, "whether this term is a synchronous reset."},
  {"is_sync_set", (PyCFunction)PySNLBitTerm_is_sync_set, METH_NOARGS, "whether this term is a synchronous set."},
  {"is_reset", (PyCFunction)PySNLBitTerm_is_reset, METH_NOARGS, "whether this term is a reset."},
  {"is_enable", (PyCFunction)PySNLBitTerm_is_enable, METH_NOARGS, "whether this term is an enable."},
  {"is_data", (PyCFunction)PySNLBitTerm_is_data, METH_NOARGS, "whether this term carries data."},
  {"is_data_input", (PyCFunction)PySNLBitTerm_is_data_input, METH_NOARGS, "whether this term is a data input."},
  {"is_data_output", (PyCFunction)PySNLBitTerm_is_data_output, METH_NOARGS, "whether this term is a data output."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyObject* PySNLBitTerm_Link(SNLBitTerm* object) {
  if (not object) {
    Py_RETURN_NONE;   
  }
  if (auto busTermBit = dynamic_cast<SNLBusTermBit*>(object)) {
    return PySNLBusTermBit_Link(busTermBit);
  } else {
    auto scalarTerm = static_cast<SNLScalarTerm*>(object);
    return PySNLScalarTerm_Link(scalarTerm);
  }
}

PyTypeNLAbstractObjectWithNLIDLinkPyType(SNLBitTerm)
PyTypeInheritedObjectDefinitions(SNLBitTerm, SNLTerm)

}
