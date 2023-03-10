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

#include "PySNLDB.h"

#include "PySNLUniverse.h"
#include "PySNLLibrary.h"

#include "SNLDB.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDB, function)

static PyObject* PySNLDB_create(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.create", &arg)) {
    setError("malformed SNLDB create");
    return nullptr;
  }
  if (not IsPySNLUniverse(arg)) {
    setError("SNLDB create should be a SNLUniverse");
    return nullptr;
  }
  auto universe = PYSNLUNIVERSE_O(arg);
  SNLDB* db = nullptr;
  SNLTRY
  db = SNLDB::create(universe);
  SNLCATCH
  return PySNLDB_Link(db);
}

GetObjectByName(DB, Library)

DBoDestroyAttribute(PySNLDB_destroy, PySNLDB)

PyMethodDef PySNLDB_Methods[] = {
  { "create", (PyCFunction)PySNLDB_create, METH_VARARGS|METH_STATIC,
    "create a SNLDB."},
  { "getLibrary", (PyCFunction)PySNLDB_getLibrary, METH_VARARGS,
    "retrieve a SNLLibrary."},
  {"destroy", (PyCFunction)PySNLDB_destroy, METH_NOARGS,
    "destroy this SNLDB."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLDB)

DBoLinkCreateMethod(SNLDB)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLDB)
PyTypeObjectDefinitions(SNLDB)

}