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

#include "PySNLInstance.h"

#include "PyInterface.h"
#include "PySNLDesign.h"

#include "SNLInstance.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLInstance, function)

static PyObject* PySNLInstance_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  const char* arg2 = nullptr;
  if (not PyArg_ParseTuple(args, "OO|s:SNLDB.create", &arg0, &arg1, &arg2)) {
    setError("malformed SNLInstance create method");
    return nullptr;
  }
  SNLName name;
  if (arg2) {
    name = SNLName(arg2);
  }

  SNLInstance* instance = nullptr;
  SNLTRY
  if (not IsPySNLDesign(arg0)) {
    setError("SNLInstance create needs SNLDesign as first argument");
    return nullptr;
  }
  if (not IsPySNLDesign(arg1)) {
    setError("SNLInstance create needs SNLDesign as second argument");
    return nullptr;
  }
  instance = SNLInstance::create(PYSNLDesign_O(arg0), PYSNLDesign_O(arg1), name);
  SNLCATCH
  return PySNLInstance_Link(instance);
}

static PyObject* PySNLInstance_getModel(PySNLInstance* self) {
  METHOD_HEAD("SNLInstance.getModel()")
  return PySNLDesign_Link(selfObject->getModel());
}

GetNameMethod(SNLInstance)

PyMethodDef PySNLInstance_Methods[] = {
  { "create", (PyCFunction)PySNLInstance_create, METH_VARARGS|METH_STATIC,
    "SNLInstance creator"},
  { "getName", (PyCFunction)PySNLInstance_getName, METH_NOARGS,
    "get SNLInstance name"},
  {"getModel", (PyCFunction)PySNLInstance_getModel, METH_NOARGS,
    "Returns the SNLInstance model SNLDesign."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLInstance_destroy, PySNLInstance)
DBoDeallocMethod(SNLInstance)

DBoLinkCreateMethod(SNLInstance)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLInstance)
PyTypeInheritedObjectDefinitions(SNLInstance, SNLDesignObject)

}
