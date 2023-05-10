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

#include "PySNLDesignObject.h"

#include "PyInterface.h"
#include "PySNLDesign.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDesignObject, function)

static PyObject* PySNLDesignObject_getDesign(PySNLDesignObject* self) {
  METHOD_HEAD("SNLDesignObject.getDesign()")
  return PySNLDesign_Link(selfObject->getDesign());
}

PyMethodDef PySNLDesignObject_Methods[] = {
  {"getDesign", (PyCFunction)PySNLDesignObject_getDesign, METH_NOARGS,
    "Returns the SNLDesignObject owner design."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLDesignObject_destroy, PySNLDesignObject)
DBoDeallocMethod(SNLDesignObject)

DBoLinkCreateMethod(SNLDesignObject)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLDesignObject)
PyTypeObjectDefinitions(SNLDesignObject)

}
