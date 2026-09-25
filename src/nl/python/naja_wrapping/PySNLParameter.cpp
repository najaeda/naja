// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLParameter.h"

#include "PyInterface.h"
#include "SNLParameter.h"
#include "PySNLDesign.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLParameter, function)

static PyObject* PySNLParameter_createString(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  const char* arg2 = nullptr;
  if (not PyArg_ParseTuple(args, "Os|s:SNLParameter.createString", &arg0, &arg1, &arg2)) {
    setError("malformed SNLParameter string value creation method");
    return nullptr;
  }
  NLName name = NLName(arg1);

  SNLParameter* parameter = nullptr;
  TRY
  if (IsPySNLDesign(arg0)) {
    parameter = arg2
      ? SNLParameter::create(PYSNLDesign_O(arg0), name, SNLParameter::Type::String, arg2)
      : SNLParameter::create(PYSNLDesign_O(arg0), name, SNLParameter::Type::String);
  } else {
    setError("SNLParameter create accepts SNLDesign as first argument");
    return nullptr;
  }
  NLCATCH
  return PySNLParameter_Link(parameter);
}

static PyObject* PySNLParameter_createDecimal(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  int value = 0;
  if (not PyArg_ParseTuple(args, "Os|i:SNLParameter.createDecimal", &arg0, &arg1, &value)) {
    setError("malformed SNLParameter int value creation method");
    return nullptr;
  }
  NLName name = NLName(arg1);

  SNLParameter* parameter = nullptr;
  TRY
  if (IsPySNLDesign(arg0)) {
    parameter = PyTuple_Size(args) > 2
      ? SNLParameter::create(PYSNLDesign_O(arg0), name, SNLParameter::Type::Decimal, std::to_string(value))
      : SNLParameter::create(PYSNLDesign_O(arg0), name, SNLParameter::Type::Decimal);
  } else {
    setError("SNLParameter create accepts SNLDesign as first argument");
    return nullptr;
  }
  NLCATCH
  return PySNLParameter_Link(parameter);
}

static PyObject* PySNLParameter_createBinary(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  int size = 0;
  int value = 0;
  if (not PyArg_ParseTuple(args, "Osi|i:SNLParameter.createBinary", &arg0, &arg1, &size, &value)) {
    setError("malformed SNLParameter binary value creation method");
    return nullptr;
  }
  NLName name = NLName(arg1);

  SNLParameter* parameter = nullptr;
  TRY
  if (IsPySNLDesign(arg0)) {
    parameter = PyTuple_Size(args) > 3
      ? SNLParameter::create(PYSNLDesign_O(arg0), name, SNLParameter::Type::Binary, std::to_string(value))
      : SNLParameter::create(PYSNLDesign_O(arg0), name, SNLParameter::Type::Binary);
  } else {
    setError("SNLParameter create accepts SNLDesign as first argument");
    return nullptr;
  }
  NLCATCH
  return PySNLParameter_Link(parameter);
}

static PyObject* PySNLParameter_createBoolean(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  int value = 0;
  if (not PyArg_ParseTuple(args, "Os|p:SNLParameter.createBoolean", &arg0, &arg1, &value)) {
    setError("malformed SNLParameter boolean value creation method");
    return nullptr;
  }
  NLName name = NLName(arg1);

  SNLParameter* parameter = nullptr;
  TRY
  if (IsPySNLDesign(arg0)) {
    parameter = PyTuple_Size(args) > 2
      ? SNLParameter::create(PYSNLDesign_O(arg0), name, SNLParameter::Type::Boolean, std::to_string(value))
      : SNLParameter::create(PYSNLDesign_O(arg0), name, SNLParameter::Type::Boolean);
  } else {
    setError("SNLParameter create accepts SNLDesign as first argument");
    return nullptr;
  }
  NLCATCH
  return PySNLParameter_Link(parameter);
}

static PyObject* PySNLParameter_hasDefaultValue(PySNLParameter* self) {
  METHOD_HEAD("PySNLParameter.hasDefaultValue()")
  return PyBool_FromLong(selfObject->hasDefaultValue());
}

static PyObject* PySNLParameter_getValue(PySNLParameter* self) {
  METHOD_HEAD("PySNLParameter.getValue()")
  if (not selfObject->hasDefaultValue()) {
    Py_RETURN_NONE;
  }
  TRY
  return PyUnicode_FromString(selfObject->getValue().c_str());
  NLCATCH
  return nullptr;
}

GetNameMethod(SNLParameter)
GetObjectMethod(SNLParameter, SNLDesign, getDesign)

DBoDestroyAttribute(PySNLParameter_destroy, PySNLParameter)

PyMethodDef PySNLParameter_Methods[] = {
  { "create_string", (PyCFunction)PySNLParameter_createString, METH_VARARGS|METH_STATIC,
    "SNLParameter string value creator; omit the final value argument for no default"},
  { "create_decimal", (PyCFunction)PySNLParameter_createDecimal, METH_VARARGS|METH_STATIC,
    "SNLParameter int value creator; omit the final value argument for no default"},
  { "create_binary", (PyCFunction)PySNLParameter_createBinary, METH_VARARGS|METH_STATIC,
    "SNLParameter binary value creator; omit the final value argument for no default"},
  { "create_boolean", (PyCFunction)PySNLParameter_createBoolean, METH_VARARGS|METH_STATIC,
    "SNLParameter boolean value creator; omit the final value argument for no default"},
  { "hasDefaultValue", (PyCFunction)PySNLParameter_hasDefaultValue, METH_NOARGS,
    "Return whether this parameter has a default value."},
  { "getValue", (PyCFunction)PySNLParameter_getValue, METH_NOARGS,
    "Return the default as a string, or None if no default exists."},
  { "getName", (PyCFunction)PySNLParameter_getName, METH_NOARGS,
    "get SNLParameter name"},
  { "getDesign", (PyCFunction)PySNLParameter_getDesign, METH_NOARGS,
    "get SNLParameter owner design"},
  { "destroy", (PyCFunction)PySNLParameter_destroy, METH_NOARGS,
    "destroy this SNLParameter."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLParameter)

DBoLinkCreateMethod(SNLParameter)
PyTypeNLObjectWithoutNLIDLinkPyType(SNLParameter)
PyTypeObjectDefinitions(SNLParameter)

}