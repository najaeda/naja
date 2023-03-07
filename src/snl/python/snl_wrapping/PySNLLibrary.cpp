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

#include "PySNLLibrary.h"

#include "PySNLDB.h"
#include "PySNLDesign.h"

#include "SNLLibrary.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLLibrary, function)

static PyObject* PySNLLibrary_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:SNLLibrary.create", &arg0, &arg1)) {
    setError("malformed SNLLibrary create");
    return nullptr;
  }
  SNLName name;
  if (arg1) {
    name = SNLName(arg1);
  }

  SNLLibrary* lib = nullptr;
  SNLTRY
  if (IsPySNLDB(arg0)) {
    lib = SNLLibrary::create(PYSNLDB_O(arg0), name);
  } else if (IsPySNLLibrary(arg0)) {
    lib = SNLLibrary::create(PYSNLLibrary_O(arg0), name);
  } else {
    setError("SNLLibrary creator accepts as first argument either a SNLDB or a SNLLibrary");
    return nullptr;
  }
  SNLCATCH
  return PySNLLibrary_Link(lib);
}

GetObjectByName(Library, Design)
GetNameMethod(SNLLibrary)

PyMethodDef PySNLLibrary_Methods[] = {
  { "create", (PyCFunction)PySNLLibrary_create, METH_VARARGS|METH_STATIC,
    "SNLLibrary creator"},
  { "getName", (PyCFunction)PySNLLibrary_getName, METH_NOARGS,
    "get SNLLibrary name"},
  { "getDesign", (PyCFunction)PySNLLibrary_getDesign, METH_VARARGS,
    "retrieve a SNLDesign."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLLibrary_destroy, PySNLLibrary)
DBoDeallocMethod(SNLLibrary)

DBoLinkCreateMethod(SNLLibrary)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLLibrary)
PyTypeObjectDefinitions(SNLLibrary)

}