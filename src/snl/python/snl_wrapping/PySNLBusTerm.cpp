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

#include "PySNLBusTerm.h"

#include "PyInterface.h"
#include "PySNLDesign.h"

#include "SNLBusTerm.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)   GENERIC_METHOD_HEAD(BusTerm, term, function)

static PyObject* PySNLBusTerm_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  int arg1 = 0;
  int arg2 = 0;
  int arg3 = 0;
  const char* arg4 = nullptr;
  if (not PyArg_ParseTuple(args, "Oiii|s:SNLDB.create", &arg0, &arg1, &arg2, &arg3, &arg4)) {
    setError("malformed SNLBusTerm create method");
    return nullptr;
  }
  SNLTerm::Direction direction = SNLNetComponent::Direction::DirectionEnum(arg1);
  SNLName name;
  if (arg4) {
    name = SNLName(arg4);
  }

  SNLBusTerm* term = nullptr;
  SNLTRY
  if (IsPySNLDesign(arg0)) {
    term = SNLBusTerm::create(PYSNLDesign_O(arg0), direction, arg2, arg3, name);
  } else {
    setError("SNLBusTerm create accepts SNLDesign as first argument");
    return nullptr;
  }
  SNLCATCH
  return PySNLBusTerm_Link(term);
}

DirectGetIntMethod(PySNLBusTerm_getMSB, getMSB, PySNLBusTerm, SNLBusTerm)
DirectGetIntMethod(PySNLBusTerm_getLSB, getLSB, PySNLBusTerm, SNLBusTerm)
DirectGetIntMethod(PySNLBusTerm_getSize, getSize, PySNLBusTerm, SNLBusTerm)


DBoLinkCreateMethod(SNLBusTerm)
DBoDeallocMethod(SNLBusTerm)
PyTypeObjectDefinitions(SNLBusTerm)

PyMethodDef PySNLBusTerm_Methods[] = {
  { "create", (PyCFunction)PySNLBusTerm_create, METH_VARARGS|METH_STATIC,
    "SNLBusTerm creator"},
  { "getMSB", (PyCFunction)PySNLBusTerm_getMSB, METH_NOARGS,
    "get SNLBusTerm MSB value"},
  { "getLSB", (PyCFunction)PySNLBusTerm_getLSB, METH_NOARGS,
    "get SNLBusTerm LSB value"},
  { "getSize", (PyCFunction)PySNLBusTerm_getSize, METH_NOARGS,
    "get SNLBusTerm Size"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeSNLObjectWithSNLIDLinkPyType(SNLBusTerm)

}
