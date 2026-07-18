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
  if (not PyArg_ParseTuple(args, "Oss:SNLParameter.createString", &arg0, &arg1, &arg2)) {
    setError("malformed SNLParameter string value creation method");
    return nullptr;
  }
  NLName name = NLName(arg1);
  std::string value = arg2;

  SNLParameter* parameter = nullptr;
  TRY
  if (IsPySNLDesign(arg0)) {
    parameter = SNLParameter::create(PYSNLDesign_O(arg0), name, SNLParameter::Type::String, value);
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
  if (not PyArg_ParseTuple(args, "Osi:SNLParameter.createDecimal", &arg0, &arg1, &value)) {
    setError("malformed SNLParameter int value creation method");
    return nullptr;
  }
  NLName name = NLName(arg1);

  SNLParameter* parameter = nullptr;
  TRY
  if (IsPySNLDesign(arg0)) {
    parameter = SNLParameter::create(PYSNLDesign_O(arg0), name, SNLParameter::Type::Decimal, std::to_string(value));
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
  if (not PyArg_ParseTuple(args, "Osii:SNLParameter.createBinary", &arg0, &arg1, &size, &value)) {
    setError("malformed SNLParameter binary value creation method");
    return nullptr;
  }
  NLName name = NLName(arg1);

  SNLParameter* parameter = nullptr;
  TRY
  if (IsPySNLDesign(arg0)) {
    if (size <= 0 || value < 0) {
      setError("SNLParameter binary value requires a positive width and non-negative value");
      return nullptr;
    }
    std::string digits(static_cast<size_t>(size), '0');
    auto unsignedValue = static_cast<unsigned int>(value);
    if (size < static_cast<int>(sizeof(unsignedValue) * 8) &&
        (unsignedValue >> size) != 0) {
      setError("SNLParameter binary value does not fit the requested width");
      return nullptr;
    }
    for (int bit = 0; bit < size; ++bit) {
      if (bit < static_cast<int>(sizeof(unsignedValue) * 8) &&
          ((unsignedValue >> bit) & 1U)) {
        digits[static_cast<size_t>(size - bit - 1)] = '1';
      }
    }
    parameter = SNLParameter::create(
      PYSNLDesign_O(arg0),
      name,
      SNLParameter::Type::Binary,
      std::to_string(size) + "'b" + digits);
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
  if (not PyArg_ParseTuple(args, "Osp:SNLParameter.createBoolean", &arg0, &arg1, &value)) {
    setError("malformed SNLParameter boolean value creation method");
    return nullptr;
  }
  NLName name = NLName(arg1);

  SNLParameter* parameter = nullptr;
  TRY
  if (IsPySNLDesign(arg0)) {
    parameter = SNLParameter::create(PYSNLDesign_O(arg0), name, SNLParameter::Type::Boolean, std::to_string(value));
  } else {
    setError("SNLParameter create accepts SNLDesign as first argument");
    return nullptr;
  }
  NLCATCH
  return PySNLParameter_Link(parameter);
}

GetNameMethod(SNLParameter)
GetObjectMethod(SNLParameter, SNLDesign, getDesign)

static PyObject* PySNLParameter_getValue(PySNLParameter* self) {
  METHOD_HEAD("SNLParameter.getValue()")
  return PyUnicode_FromString(selfObject->getValue().c_str());
}

static PyObject* PySNLParameter_getType(PySNLParameter* self) {
  METHOD_HEAD("SNLParameter.getType()")
  return PyLong_FromLong(static_cast<long>(
    static_cast<SNLParameter::Type::TypeEnum>(selfObject->getType())));
}

DBoDestroyAttribute(PySNLParameter_destroy, PySNLParameter)

PyMethodDef PySNLParameter_Methods[] = {
  { "create_string", (PyCFunction)PySNLParameter_createString, METH_VARARGS|METH_STATIC,
    "SNLParameter string value creator"},
  { "create_decimal", (PyCFunction)PySNLParameter_createDecimal, METH_VARARGS|METH_STATIC,
    "SNLParameter int value creator"},
  { "create_binary", (PyCFunction)PySNLParameter_createBinary, METH_VARARGS|METH_STATIC,
    "SNLParameter binary value creator"},
  { "create_boolean", (PyCFunction)PySNLParameter_createBoolean, METH_VARARGS|METH_STATIC,
    "SNLParameter boolean value creator"},
  { "getName", (PyCFunction)PySNLParameter_getName, METH_NOARGS,
    "get SNLParameter name"},
  { "getDesign", (PyCFunction)PySNLParameter_getDesign, METH_NOARGS,
    "get SNLParameter owner design"},
  { "getValue", (PyCFunction)PySNLParameter_getValue, METH_NOARGS,
    "get the parameter value string"},
  { "getType", (PyCFunction)PySNLParameter_getType, METH_NOARGS,
    "get the SNLParameter type tag"},
  { "destroy", (PyCFunction)PySNLParameter_destroy, METH_NOARGS,
    "destroy this SNLParameter."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLParameter)

DBoLinkCreateMethod(SNLParameter)
PyTypeNLObjectWithoutNLIDLinkPyType(SNLParameter)
PyTypeObjectDefinitions(SNLParameter)

}
