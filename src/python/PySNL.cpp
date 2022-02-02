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
#include "PySNLDB.h"
#include "PySNLLibrary.h"
#include "PySNLDesign.h"
#include "PySNLBusNet.h"
#include "PySNLScalarNet.h"
#include "PySNLBusNetBit.h"
#include "PySNLScalarTerm.h"
#include "PySNLBusTermBit.h"
#include "PySNLBusTerm.h"
#include "PySNLInstance.h"

namespace PYSNL {

static PyMethodDef SNLMethods[] = {
  {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef snlModule = {
  PyModuleDef_HEAD_INIT,
  "snl",
  "Python interface for SNL netlist API",
  -1,
  SNLMethods
};

PyMODINIT_FUNC PyInit_snl(void) {
  PySNLUniverse_LinkPyType();
  PySNLDB_LinkPyType();
  PySNLLibrary_LinkPyType();
  PySNLDesign_LinkPyType();
  PySNLDesignObject_LinkPyType ();
  PySNLNet_LinkPyType();
  PySNLBusNet_LinkPyType();
  PySNLBitNet_LinkPyType();
  PySNLScalarNet_LinkPyType();
  PySNLBusNetBit_LinkPyType();
  PySNLNetComponent_LinkPyType ();
  PySNLTerm_LinkPyType();
  PySNLBusTerm_LinkPyType();
  PySNLBitTerm_LinkPyType();
  PySNLScalarTerm_LinkPyType();
  PySNLBusTermBit_LinkPyType();
  PySNLInstance_LinkPyType();

  PYTYPE_READY(SNLUniverse);
  PYTYPE_READY(SNLDB);
  PYTYPE_READY(SNLLibrary);
  PYTYPE_READY(SNLDesign);
  PYTYPE_READY(SNLDesignObject);
  PYTYPE_READY_SUB(SNLNet, SNLDesignObject);
  PYTYPE_READY_SUB(SNLBusNet, SNLNet);
  PYTYPE_READY_SUB(SNLBitNet, SNLNet);
  PYTYPE_READY_SUB(SNLScalarNet, SNLBitNet);
  PYTYPE_READY_SUB(SNLBusNetBit, SNLBitNet);
  PYTYPE_READY_SUB(SNLNetComponent, SNLDesignObject);
  PYTYPE_READY_SUB(SNLTerm, SNLNetComponent);
  PYTYPE_READY_SUB(SNLBusTerm, SNLTerm);
  PYTYPE_READY_SUB(SNLBitTerm, SNLTerm);
  PYTYPE_READY_SUB(SNLScalarTerm, SNLBitTerm);
  PYTYPE_READY_SUB(SNLBusTermBit, SNLBitTerm);
  PYTYPE_READY_SUB(SNLInstance, SNLDesignObject);

  Py_INCREF(&PyTypeSNLUniverse);
  Py_INCREF(&PyTypeSNLDB);
  Py_INCREF(&PyTypeSNLLibrary);
  Py_INCREF(&PyTypeSNLDesign);
  Py_INCREF(&PyTypeSNLDesignObject);
  Py_INCREF(&PyTypeSNLNet);
  Py_INCREF(&PyTypeSNLBusNet);
  Py_INCREF(&PyTypeSNLBitNet);
  Py_INCREF(&PyTypeSNLScalarNet);
  Py_INCREF(&PyTypeSNLBusNetBit);
  Py_INCREF(&PyTypeSNLNetComponent);
  Py_INCREF(&PyTypeSNLTerm);
  Py_INCREF(&PyTypeSNLBusTerm);
  Py_INCREF(&PyTypeSNLBitTerm);
  Py_INCREF(&PyTypeSNLScalarTerm);
  Py_INCREF(&PyTypeSNLBusTermBit);
  Py_INCREF(&PyTypeSNLInstance);

  PyObject* mod = PyModule_Create(&snlModule);

  if (not mod) {
    std::cerr << "[ERROR]\n"
      << "  Failed to initialize SNL python module." << std::endl;
    return nullptr;
  }

  PyModule_AddObject(mod, "SNLUniverse", (PyObject*)&PyTypeSNLUniverse);
  PyModule_AddObject(mod, "SNLDB", (PyObject*)&PyTypeSNLDB);
  PyModule_AddObject(mod, "SNLLibrary", (PyObject*)&PyTypeSNLLibrary);
  PyModule_AddObject(mod, "SNLDesign", (PyObject*)&PyTypeSNLDesign);
  PyModule_AddObject(mod, "SNLDesignObject", (PyObject*)&PyTypeSNLDesignObject);
  PyModule_AddObject(mod, "SNLNet", (PyObject*)&PyTypeSNLNet);
  PyModule_AddObject(mod, "SNLBusNet", (PyObject*)&PyTypeSNLBusNet);
  PyModule_AddObject(mod, "SNLBitNet", (PyObject*)&PyTypeSNLBitNet);
  PyModule_AddObject(mod, "SNLScalarNet", (PyObject*)&PyTypeSNLScalarNet);
  PyModule_AddObject(mod, "SNLBusNetBit", (PyObject*)&PyTypeSNLBusNetBit);
  PyModule_AddObject(mod, "SNLNetComponent", (PyObject*)&PyTypeSNLNetComponent);
  PyModule_AddObject(mod, "SNLTerm", (PyObject*)&PyTypeSNLTerm);
  PyModule_AddObject(mod, "SNLBusTerm", (PyObject*)&PyTypeSNLBusTerm);
  PyModule_AddObject(mod, "SNLBitTerm", (PyObject*)&PyTypeSNLBitTerm);
  PyModule_AddObject(mod, "SNLScalarTerm", (PyObject*)&PyTypeSNLScalarTerm);
  PyModule_AddObject(mod, "SNLBusTermBit", (PyObject*)&PyTypeSNLBusTermBit);
  PyModule_AddObject(mod, "SNLInstance", (PyObject*)&PyTypeSNLInstance);

  return mod;
}

}
