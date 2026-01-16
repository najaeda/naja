// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>

#include "NajaLog.h"

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
#include "PySNLEquipotential.h"
#include "PySNLOccurrences.h"
#include "PySNLUniquifier.h"

#include "NajaVersion.h"

namespace PYNAJA {

static PyObject* getVersion(PyObject* self, PyObject* args) {
  return PyUnicode_FromString(naja::NAJA_VERSION.c_str());
}

static PyObject* getGitHash(PyObject* self, PyObject* args) {
  return PyUnicode_FromString(naja::NAJA_GIT_HASH.c_str());
}

static bool parseLogLevel_(const char* levelName,
                           spdlog::level::level_enum& level) {
  std::string levelStr(levelName ? levelName : "");
  level = naja::log::levelFromString(levelStr);
  std::string lowered = levelStr;
  std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (level == spdlog::level::off && lowered != "off") {
    return false;
  }
  return true;
}

static PyObject* logInfo(PyObject* self, PyObject* args) {
  const char* message;

  if (!PyArg_ParseTuple(args, "s", &message)) {
    setError("Failed to parse arguments in logInfo");
    return nullptr;
  }
  NAJA_LOG_INFO("{}", message);
  Py_RETURN_NONE;
}

static PyObject* logCritical(PyObject* self, PyObject* args) {
  const char* message;
  if (!PyArg_ParseTuple(args, "s", &message)) {
    setError("Failed to parse arguments in logCritical");
    return nullptr;
  }
  NAJA_LOG_CRITICAL("{}", message);
  Py_RETURN_NONE;
}

static PyObject* logWarn(PyObject* self, PyObject* args) {
  const char* message;
  if (!PyArg_ParseTuple(args, "s", &message)) {
    setError("Failed to parse arguments in logWarning");
    return nullptr;
  }
  NAJA_LOG_WARN("{}", message);
  Py_RETURN_NONE;
}

static PyObject* setLogLevel(PyObject* self, PyObject* args) {
  const char* levelName = nullptr;
  if (!PyArg_ParseTuple(args, "s", &levelName)) {
    setError("Failed to parse arguments in setLogLevel");
    return nullptr;
  }
  spdlog::level::level_enum level;
  if (!parseLogLevel_(levelName, level)) {
    setError("Invalid log level");
    return nullptr;
  }
  naja::log::setLevel(level);
  Py_RETURN_NONE;
}

static PyObject* addLogFile(PyObject* self, PyObject* args) {
  const char* path = nullptr;
  const char* levelName = nullptr;
  if (!PyArg_ParseTuple(args, "s|s", &path, &levelName)) {
    setError("Failed to parse arguments in addLogFile");
    return nullptr;
  }
  std::ofstream testStream(path, std::ios::app);
  if (!testStream.is_open()) {
    NAJA_LOG_WARN("Log file not writable: {}", path);
    Py_RETURN_NONE;
  }
  testStream.close();
  spdlog::level::level_enum level = spdlog::level::trace;
  if (levelName && !parseLogLevel_(levelName, level)) {
    setError("Invalid log level");
    return nullptr;
  }
  naja::log::addFileSink(path, level);
  Py_RETURN_NONE;
}

static PyObject* clearLogSinks(PyObject* self, PyObject* args) {
  naja::log::clearSinks();
  Py_RETURN_NONE;
}

static PyMethodDef NajaMethods[] = {
  { "getVersion", getVersion, METH_NOARGS, "get the version of Naja" },
  { "getGitHash", getGitHash, METH_NOARGS, "get the Naja git hash" },
  { "logInfo", logInfo, METH_VARARGS, "log an info message" },
  { "logWarn", logWarn, METH_VARARGS, "log a warning message" },
  { "logCritical", logCritical, METH_VARARGS, "log a critical message" },
  { "setLogLevel", setLogLevel, METH_VARARGS, "set the global log level" },
  { "addLogFile", addLogFile, METH_VARARGS, "add a file sink to the logger" },
  { "clearLogSinks", clearLogSinks, METH_NOARGS, "clear all log sinks" },
  {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef najaModule = {
  PyModuleDef_HEAD_INIT,
  "naja",
  "Python interface for Naja API",
  -1,
  NajaMethods
};

PyMODINIT_FUNC PyInit_naja(void) {
  naja::log::initFromEnv();

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
  PySNLOccurrences_LinkPyType();

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
  PYTYPE_READY(SNLOccurrences);
  PYTYPE_READY(SNLOccurrencesIterator);

  PyObject* mod = PyModule_Create(&najaModule);

  if (not mod) {
    //LCOV_EXCL_START
    NAJA_LOG_ERROR("Failed to initialize Naja python module.");
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
  PyModule_AddType(mod, &PyTypeSNLEquipotential);

  PySNLTerm_postModuleInit();
  PySNLNet_postModuleInit();

  return mod;
}

}
