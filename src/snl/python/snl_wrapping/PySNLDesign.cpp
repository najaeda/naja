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

#include "PySNLDesign.h"

#include "PySNLLibrary.h"

#include "SNLDesign.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDesign, design, function)

static PyObject* PySNLDesign_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:SNLDB.create", &arg0, &arg1)) {
    setError("malformed SNLDesign create method");
    return nullptr;
  }
  SNLName name;
  if (arg1) {
    name = SNLName(arg1);
  }

  SNLDesign* design = nullptr;
  SNLTRY
  if (IsPySNLLibrary(arg0)) {
    design = SNLDesign::create(PYSNLLibrary_O(arg0), name);
  } else {
    setError("SNLDesign create accepts SNLLibrary as first argument");
    return nullptr;
  }
  SNLCATCH
  return PySNLDesign_Link(design);
}

static PyObject* PySNLDesign_getLibrary(PySNLDesign* self) {
  METHOD_HEAD("SNLDesign.getLibrary()")
  return PySNLLibrary_Link(design->getLibrary());
}

GetNameMethod(SNLDesign, design)

DBoDestroyAttribute(PySNLDesign_destroy, PySNLDesign)

PyMethodDef PySNLDesign_Methods[] = {
  { "create", (PyCFunction)PySNLDesign_create, METH_VARARGS|METH_STATIC,
    "SNLDesign creator"},
  { "getName", (PyCFunction)PySNLDesign_getName, METH_NOARGS,
    "get SNLDesign name"},
  {"getLibrary", (PyCFunction)PySNLDesign_getLibrary, METH_NOARGS,
    "Returns the SNLDesign owner SNLLibrary."},
  {"destroy", (PyCFunction)PySNLDesign_destroy, METH_NOARGS,
    "destroy this SNLDesign."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLDesign)

DBoLinkCreateMethod(SNLDesign)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLDesign)
PyTypeObjectDefinitions(SNLDesign)

}
