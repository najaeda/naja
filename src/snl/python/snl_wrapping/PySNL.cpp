// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLUniverse.h"
#include "PySNLDB.h"
#include "PySNLLibrary.h"
#include "PySNLDesign.h"
#include "PySNLParameter.h"
#include "PySNLBusNet.h"
#include "PySNLScalarNet.h"
#include "PySNLBusNetBit.h"
#include "PySNLTermDirection.h"
#include "PySNLScalarTerm.h"
#include "PySNLBusTermBit.h"
#include "PySNLBusTerm.h"
#include "PySNLInstance.h"
#include "PySNLInstTerm.h"
#include "PySNLLibraries.h"
#include "PySNLDesigns.h"
#include "PySNLTerms.h"
#include "PySNLBitTerms.h"
#include "PySNLScalarTerms.h"
#include "PySNLBusTerms.h"
#include "PySNLNets.h"
#include "PySNLBitNets.h"
#include "PySNLInstances.h"
#include "PySNLInstTerms.h"

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
  PySNLParameter_LinkPyType();
  PySNLDesignObject_LinkPyType ();
  PySNLNet_LinkPyType();
  PySNLBusNet_LinkPyType();
  PySNLBitNet_LinkPyType();
  PySNLScalarNet_LinkPyType();
  PySNLBusNetBit_LinkPyType();
  PySNLNetComponent_LinkPyType ();
  PySNLTerm_LinkPyType();
  PySNLTermDirection_LinkPyType();
  PySNLBusTerm_LinkPyType();
  PySNLBitTerm_LinkPyType();
  PySNLScalarTerm_LinkPyType();
  PySNLBusTermBit_LinkPyType();
  PySNLInstance_LinkPyType();
  PySNLInstTerm_LinkPyType();

  PySNLLibraries_LinkPyType();
  PySNLDesigns_LinkPyType();
  PySNLTerms_LinkPyType();
  PySNLBitTerms_LinkPyType();
  PySNLScalarTerms_LinkPyType();
  PySNLBusTerms_LinkPyType();
  PySNLNets_LinkPyType();
  PySNLBitNets_LinkPyType();
  PySNLInstances_LinkPyType();
  PySNLInstTerms_LinkPyType();

  PYTYPE_READY(SNLUniverse);
  PYTYPE_READY(SNLDB);
  PYTYPE_READY(SNLLibrary);
  PYTYPE_READY(SNLDesign);
  PYTYPE_READY(SNLParameter);
  PYTYPE_READY(SNLDesignObject);
  PYTYPE_READY(SNLTermDirection);
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
  PYTYPE_READY_SUB(SNLInstTerm, SNLNetComponent);

  PYTYPE_READY(SNLLibraries);
  PYTYPE_READY(SNLLibrariesIterator);
  PYTYPE_READY(SNLDesigns);
  PYTYPE_READY(SNLDesignsIterator);
  PYTYPE_READY(SNLTerms);
  PYTYPE_READY(SNLTermsIterator);
  PYTYPE_READY(SNLBitTerms);
  PYTYPE_READY(SNLBitTermsIterator);
  PYTYPE_READY(SNLScalarTerms);
  PYTYPE_READY(SNLScalarTermsIterator);
  PYTYPE_READY(SNLBusTerms);
  PYTYPE_READY(SNLBusTermsIterator);
  PYTYPE_READY(SNLNets);
  PYTYPE_READY(SNLNetsIterator);
  PYTYPE_READY(SNLBitNets);
  PYTYPE_READY(SNLBitNetsIterator);
  PYTYPE_READY(SNLInstances);
  PYTYPE_READY(SNLInstancesIterator);
  PYTYPE_READY(SNLInstTerms);
  PYTYPE_READY(SNLInstTermsIterator);

  //FIXME:XTOF Why those increfs ??
  Py_INCREF(&PyTypeSNLUniverse);
  Py_INCREF(&PyTypeSNLDB);
  Py_INCREF(&PyTypeSNLLibrary);
  Py_INCREF(&PyTypeSNLDesign);
  Py_INCREF(&PyTypeSNLParameter);
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
  Py_INCREF(&PyTypeSNLInstTerm);
  Py_INCREF(&PyTypeSNLLibraries);
  Py_INCREF(&PyTypeSNLDesigns);
  Py_INCREF(&PyTypeSNLTerms);
  Py_INCREF(&PyTypeSNLBitTerms);
  Py_INCREF(&PyTypeSNLScalarTerms);
  Py_INCREF(&PyTypeSNLBusTerms);
  Py_INCREF(&PyTypeSNLNets);
  Py_INCREF(&PyTypeSNLBitNets);
  Py_INCREF(&PyTypeSNLInstances);
  Py_INCREF(&PyTypeSNLInstTerms);

  PyObject* mod = PyModule_Create(&snlModule);

  if (not mod) {
    //LCOV_EXCL_START
    std::cerr << "[ERROR]\n"
      << "  Failed to initialize SNL python module." << std::endl;
    return nullptr;
    //LCOV_EXCL_STOP
  }

  PyModule_AddObject(mod, "SNLUniverse", (PyObject*)&PyTypeSNLUniverse);
  PyModule_AddObject(mod, "SNLDB", (PyObject*)&PyTypeSNLDB);
  PyModule_AddObject(mod, "SNLLibrary", (PyObject*)&PyTypeSNLLibrary);
  PyModule_AddObject(mod, "SNLDesign", (PyObject*)&PyTypeSNLDesign);
  PyModule_AddObject(mod, "SNLParameter", (PyObject*)&PyTypeSNLParameter);
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
  PyModule_AddObject(mod, "SNLInstTerm", (PyObject*)&PyTypeSNLInstTerm);

  PySNLTerm_postModuleInit();

  return mod;
}

}
