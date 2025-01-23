// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLAttribute.h"
#include "PyInterface.h"

#include "SNLAttributes.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLAttribute, function)

static int PySNLAttribute_Init(PySNLAttribute* self, PyObject* args, PyObject* kwargs) {
  SNLAttribute* snlAttribute = nullptr;
  char* arg0 = nullptr;
  PyObject* arg1 = nullptr;

  if (not PyArg_ParseTuple(args, "s|O:SNLAttribute", &arg0, &arg1)) {
    setError("malformed SNLAttribute constructor method");
    return -1;
  }
  if (arg1 == nullptr) {
    snlAttribute = new SNLAttribute(SNLName(arg0));
  } else if (PyUnicode_Check(arg1)) {
    snlAttribute = new SNLAttribute(
      SNLName(arg0),
      SNLAttributeValue(PyUnicode_AsUTF8(arg1))
    );
  } else if (PyLong_Check(arg1) or PyFloat_Check(arg1)) {
    // Convert PyLong to string
    PyObject* py_string = PyObject_Str(arg1);
    if (!py_string) {
      //LCOV_EXCL_START
      setError("Failed to convert Attribute numerical value to string");
      return -1;
      //LCOV_EXCL_STOP
    }
    // Convert Python string to UTF-8 encoded C string
    const char* c_string = PyUnicode_AsUTF8(py_string);
    snlAttribute = new SNLAttribute(
      SNLName(arg0),
      SNLAttributeValue(
        SNLAttributeValue::Type::NUMBER,
        std::string(c_string))
    );
    // Decrement the reference count of py_string
    Py_DECREF(py_string);
  }  else {
    setError("invalid number of parameters for SNLAttribute constructor.");
    return -1;
  }
  self->object_ = snlAttribute;
  return 0;
}

ManagedTypeLinkCreateMethod(SNLAttribute) 
ManagedTypeDeallocMethod(SNLAttribute)

GetNameMethod(SNLAttribute)
GetBoolAttribute(Attribute, hasValue)

static PyObject* PySNLAttribute_getValue(PySNLAttribute* self) {
  METHOD_HEAD("SNLAttribute.getValue()")
  return PyUnicode_FromString(selfObject->getValue().getString().c_str());
}

PyMethodDef PySNLAttribute_Methods[] = {
  { "getName", (PyCFunction)PySNLAttribute_getName, METH_NOARGS,
    "get the name of the Attribute."},
  { "hasValue", (PyCFunction)PySNLAttribute_hasValue, METH_NOARGS,
    "check if the Attribute has a value."},
  { "getValue", (PyCFunction)PySNLAttribute_getValue, METH_NOARGS,
    "get the value of the Attribute."},
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedSNLObjectWithoutSNLIDLinkPyType(SNLAttribute)
PyTypeObjectDefinitions(SNLAttribute)

}  // namespace PYSNL
