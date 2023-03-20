/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PySNLParameter.h"

#include "SNLParameter.h"
#include "PySNLDesign.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLParameter, function)

static PyObject* PySNLParameter_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  const char* arg2 = nullptr;
  if (not PyArg_ParseTuple(args, "Oss:SNLParameter.create", &arg0, &arg1, &arg2)) {
    setError("malformed SNLParameter create method");
    return nullptr;
  }
  SNLName name = SNLName(arg1);
  std::string value = arg2;

  SNLParameter* parameter = nullptr;
  SNLTRY
  if (IsPySNLDesign(arg0)) {
    parameter = SNLParameter::create(PYSNLDesign_O(arg0), name, value);
  } else {
    setError("SNLParameter create accepts SNLDesign as first argument");
    return nullptr;
  }
  SNLCATCH
  return PySNLParameter_Link(parameter);
}

GetNameMethod(SNLParameter)

DBoDestroyAttribute(PySNLParameter_destroy, PySNLParameter)

PyMethodDef PySNLParameter_Methods[] = {
  { "create", (PyCFunction)PySNLParameter_create, METH_VARARGS|METH_STATIC,
    "SNLDesign creator"},
  { "getName", (PyCFunction)PySNLParameter_getName, METH_NOARGS,
    "get SNLDesign name"},
  {"destroy", (PyCFunction)PySNLParameter_destroy, METH_NOARGS,
    "destroy this SNLDesign."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLParameter)

DBoLinkCreateMethod(SNLParameter)
PyTypeSNLObjectWithoutSNLIDLinkPyType(SNLParameter)
PyTypeObjectDefinitions(SNLParameter)

}