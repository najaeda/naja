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

#include "PySNLUniverse.h"

#include "SNLUniverse.h"

namespace PYSNL {

using namespace SNL;

static PyObject* PySNLUniverse_create() {
  SNL::SNLUniverse* universe = nullptr;
  SNLTRY
  universe = SNL::SNLUniverse::create();
  SNLCATCH
  return PySNLUniverse_Link(universe);
}

static PyObject* PySNLUniverse_get() {
  auto universe = SNL::SNLUniverse::get();
  return PySNLUniverse_Link(universe);
}

DBoDestroyAttribute(PySNLUniverse_destroy, PySNLUniverse)

PyMethodDef PySNLUniverse_Methods[] = {
  { "create", (PyCFunction)PySNLUniverse_create, METH_NOARGS|METH_STATIC,
    "create the SNL Universe (static object)"},
  { "destroy", (PyCFunction)PySNLUniverse_destroy, METH_NOARGS,
    "destroy the associated SNLUniverse"},
  { "get", (PyCFunction)PySNLUniverse_get, METH_NOARGS|METH_STATIC,
    "get the SNL Universe (static object)"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLUniverse)

DBoLinkCreateMethod(SNLUniverse)
PyTypeObjectLinkPyType(SNLUniverse)
PyTypeObjectDefinitions(SNLUniverse)

}
