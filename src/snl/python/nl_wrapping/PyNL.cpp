// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#include "PyNLUniverse.h"
#include "PyNLDB.h"
#include "PyNLLibrary.h"
#include "PySNLAttribute.h"
#include "PySNLDesign.h"
#include "PySNLParameter.h"
#include "PySNLNetType.h"
#include "PySNLBusNet.h"
#include "PySNLScalarNet.h"
#include "PySNLBusNetBit.h"
#include "PySNLTermDirection.h"
#include "PySNLScalarTerm.h"
#include "PySNLBusTermBit.h"
#include "PySNLBusTerm.h"
#include "PySNLInstance.h"
#include "PySNLInstParameter.h"
#include "PySNLInstTerm.h"
#include "PySNLAttributes.h"
#include "PyNLDBs.h"
#include "PyNLLibraries.h"
#include "PySNLDesigns.h"
#include "PySNLParameters.h"
#include "PySNLInstParameters.h"
#include "PySNLTerms.h"
#include "PySNLBitTerms.h"
#include "PySNLScalarTerms.h"
#include "PySNLBusTerms.h"
#include "PySNLNets.h"
#include "PySNLBitNets.h"
#include "PySNLScalarNets.h"
#include "PySNLBusNets.h"
#include "PySNLNetComponents.h"
#include "PySNLInstances.h"
#include "PySNLInstTerms.h"
#include "PySNLPath.h"
#include "PySNLOccurrence.h"
#include "PySNLInstTermOccurrence.h"
#include "PySNLNetComponentOccurrence.h"
#include "PySNLEquipotential.h"
#include "PySNLInstTermOccurrences.h"
#include "PySNLUniquifier.h"

#include "NajaVersion.h"

namespace PYSNL {

static PyObject* getVersion(PyObject* self, PyObject* args) {
  return PyUnicode_FromString(naja::NAJA_VERSION.c_str());
}

static PyObject* getGitVersion(PyObject* self, PyObject* args) {
  return PyUnicode_FromString(naja::NAJA_GIT_HASH.c_str());
}

static PyMethodDef SNLMethods[] = {
  { "getVersion", getVersion, METH_NOARGS, "get the version of SNL" },
  { "getGitVersion", getGitVersion, METH_NOARGS, "get the naja git hash" },
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
  PyNLUniverse_LinkPyType();
  PyNLDB_LinkPyType();
  PyNLLibrary_LinkPyType();
  PySNLAttribute_LinkPyType();
  PySNLDesign_LinkPyType();
  PySNLParameter_LinkPyType();
  PySNLDesignObject_LinkPyType ();
  PySNLNet_LinkPyType();
  PySNLNetType_LinkPyType();
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
  PySNLInstParameter_LinkPyType();
  PySNLInstTerm_LinkPyType();
  PySNLPath_LinkPyType();
  PySNLUniquifier_LinkPyType();
  PySNLEquipotential_LinkPyType();
  PySNLOccurrence_LinkPyType();
  PySNLNetComponentOccurrence_LinkPyType();
  PySNLInstTermOccurrence_LinkPyType();

  PySNLAttributes_LinkPyType();
  PyNLDBs_LinkPyType();
  PyNLLibraries_LinkPyType();
  PySNLDesigns_LinkPyType();
  PySNLParameters_LinkPyType();
  PySNLTerms_LinkPyType();
  PySNLBitTerms_LinkPyType();
  PySNLScalarTerms_LinkPyType();
  PySNLBusTerms_LinkPyType();
  PySNLNets_LinkPyType();
  PySNLBitNets_LinkPyType();
  PySNLScalarNets_LinkPyType();
  PySNLBusNets_LinkPyType();
  PySNLNetComponents_LinkPyType();
  PySNLInstances_LinkPyType();
  PySNLInstParameters_LinkPyType();
  PySNLInstTerms_LinkPyType();
  PySNLInstTermOccurrences_LinkPyType();

  PYTYPE_READY(SNLAttribute);
  PYTYPE_READY(NLUniverse);
  PYTYPE_READY(NLDB);
  PYTYPE_READY(NLLibrary);
  PYTYPE_READY(SNLDesign);
  PYTYPE_READY(SNLParameter);
  PYTYPE_READY(SNLInstParameter);
  PYTYPE_READY(SNLDesignObject);
  PYTYPE_READY(SNLTermDirection);
  PYTYPE_READY_SUB(SNLNet, SNLDesignObject);
  PYTYPE_READY(SNLNetType);
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

  PYTYPE_READY(SNLPath);
  PYTYPE_READY(SNLUniquifier);
  PYTYPE_READY(SNLEquipotential);
  PYTYPE_READY(SNLOccurrence);
  PYTYPE_READY(SNLNetComponentOccurrence);
  PYTYPE_READY(SNLInstTermOccurrence);
  PYTYPE_READY(SNLAttributes);
  PYTYPE_READY(NLDBs);
  PYTYPE_READY(NLDBsIterator);
  PYTYPE_READY(NLLibraries);
  PYTYPE_READY(NLLibrariesIterator);
  PYTYPE_READY(SNLDesigns);
  PYTYPE_READY(SNLDesignsIterator);
  PYTYPE_READY(SNLParameters);
  PYTYPE_READY(SNLParametersIterator);
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
  PYTYPE_READY(SNLScalarNets);
  PYTYPE_READY(SNLScalarNetsIterator);
  PYTYPE_READY(SNLBusNets);
  PYTYPE_READY(SNLBusNetsIterator);
  PYTYPE_READY(SNLNetComponents);
  PYTYPE_READY(SNLNetComponentsIterator);
  PYTYPE_READY(SNLInstances);
  PYTYPE_READY(SNLInstancesIterator);
  PYTYPE_READY(SNLInstParameters);
  PYTYPE_READY(SNLInstParametersIterator);
  PYTYPE_READY(SNLInstTerms);
  PYTYPE_READY(SNLInstTermsIterator);
  PYTYPE_READY(SNLInstTermOccurrences);
  PYTYPE_READY(SNLInstTermOccurrencesIterator);

  PyObject* mod = PyModule_Create(&snlModule);

  if (not mod) {
    //LCOV_EXCL_START
    std::cerr << "[ERROR]\n"
      << "  Failed to initialize SNL python module." << std::endl;
    return nullptr;
    //LCOV_EXCL_STOP
  }

  PyModule_AddType(mod, &PyTypeSNLAttribute);
  PyModule_AddType(mod, &PyTypeNLUniverse);
  PyModule_AddType(mod, &PyTypeNLDB);
  PyModule_AddType(mod, &PyTypeNLLibrary);
  PyModule_AddType(mod, &PyTypeSNLDesign);
  PyModule_AddType(mod, &PyTypeSNLParameter);
  PyModule_AddType(mod, &PyTypeSNLDesignObject);
  PyModule_AddType(mod, &PyTypeSNLNet);
  PyModule_AddType(mod, &PyTypeSNLBusNet);
  PyModule_AddType(mod, &PyTypeSNLBitNet);
  PyModule_AddType(mod, &PyTypeSNLScalarNet);
  PyModule_AddType(mod, &PyTypeSNLBusNetBit);
  PyModule_AddType(mod, &PyTypeSNLNetComponent);
  PyModule_AddType(mod, &PyTypeSNLTerm);
  PyModule_AddType(mod, &PyTypeSNLBusTerm);
  PyModule_AddType(mod, &PyTypeSNLBitTerm);
  PyModule_AddType(mod, &PyTypeSNLScalarTerm);
  PyModule_AddType(mod, &PyTypeSNLBusTermBit);
  PyModule_AddType(mod, &PyTypeSNLInstance);
  PyModule_AddType(mod, &PyTypeSNLInstParameter);
  PyModule_AddType(mod, &PyTypeSNLInstTerm);
  PyModule_AddType(mod, &PyTypeSNLPath);
  PyModule_AddType(mod, &PyTypeSNLUniquifier);
  PyModule_AddType(mod, &PyTypeSNLOccurrence);
  PyModule_AddType(mod, &PyTypeSNLNetComponentOccurrence);
  PyModule_AddType(mod, &PyTypeSNLInstTermOccurrence);
  PyModule_AddType(mod, &PyTypeSNLEquipotential);

  PySNLTerm_postModuleInit();
  PySNLNet_postModuleInit();

  return mod;
}

}
