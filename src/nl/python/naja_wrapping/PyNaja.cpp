// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "NajaLog.h"
#include "NajaPerf.h"

#include "PyNLUniverse.h"
#include "PyNLID.h"
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
#include "PySNLBundleTerm.h"
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
#include "PySNLBundleTerms.h"
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
#include "PySNLLogicalCone.h"
#include "PySNLOccurrences.h"
#include "PySNLUniquifier.h"

#include "NajaVersion.h"
#include "NLObject.h"
#include "SNLSVConstructor.h"
#include "SNLSVIntent.h"
#include "SNLDesign.h"
#include "SNLDesignObject.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLNetComponent.h"
#include "SNLTerm.h"

namespace PYNAJA {

namespace {

constexpr const char* kFrontendSymbolCapsuleName = "naja.frontend.Symbol";
constexpr const char* kFrontendCompilationCapsuleName = "naja.frontend.Compilation";

PyObject* wrapNLObject(naja::NL::NLObject* object) {
  using namespace naja::NL;
  if (!object) {
    Py_RETURN_NONE;
  }
  if (auto* design = dynamic_cast<SNLDesign*>(object)) {
    return PySNLDesign_Link(design);
  }
  if (auto* instance = dynamic_cast<SNLInstance*>(object)) {
    return PySNLInstance_Link(instance);
  }
  if (auto* net = dynamic_cast<SNLNet*>(object)) {
    return PySNLNet_Link(net);
  }
  if (auto* term = dynamic_cast<SNLTerm*>(object)) {
    return PySNLTerm_Link(term);
  }
  if (auto* component = dynamic_cast<SNLNetComponent*>(object)) {
    return PySNLNetComponent_Link(component);
  }
  Py_RETURN_NONE;
}

std::string formatIntentSourceLoc(const naja::NL::SNLSourceLoc& loc) {
  if (loc.line == 0 || loc.file.empty()) {
    return {};
  }
  const auto filename = std::filesystem::path(loc.file.getString()).filename().string();
  return filename + ":" + std::to_string(loc.line);
}

int setDictItem(PyObject* dict, const char* key, PyObject* value) {
  if (!value) {
    return -1;
  }
  const int status = PyDict_SetItemString(dict, key, value);
  Py_DECREF(value);
  return status;
}

PyObject* buildIntentParamDict(const naja::NL::SNLSVIntentParam& param) {
  if (param.name.empty()) {
    Py_RETURN_NONE;
  }
  PyObject* dict = PyDict_New();
  if (!dict) {
    return nullptr;
  }
  const auto src = formatIntentSourceLoc(param.loc);
  if (setDictItem(dict, "name", PyUnicode_FromString(param.name.c_str())) < 0 ||
      setDictItem(dict, "value", PyUnicode_FromString(param.value.c_str())) < 0 ||
      setDictItem(dict, "expr", PyUnicode_FromString(param.expr.c_str())) < 0 ||
      setDictItem(dict, "localparam", PyBool_FromLong(param.localParam)) < 0 ||
      setDictItem(dict, "src", PyUnicode_FromString(src.c_str())) < 0) {
    Py_DECREF(dict);
    return nullptr;
  }
  return dict;
}

PyObject* buildIntentTypeDict(const naja::NL::SNLSVIntentType& type) {
  if (!type.valid) {
    Py_RETURN_NONE;
  }
  PyObject* dict = PyDict_New();
  if (!dict) {
    return nullptr;
  }
  const auto src = formatIntentSourceLoc(type.declLoc);
  if (setDictItem(dict, "type", PyUnicode_FromString(type.typeName.c_str())) < 0 ||
      setDictItem(
        dict,
        "canonical_kind",
        PyUnicode_FromString(type.canonicalKind.c_str())) < 0 ||
      setDictItem(dict, "src", PyUnicode_FromString(src.c_str())) < 0) {
    Py_DECREF(dict);
    return nullptr;
  }
  if (type.isEnum) {
    PyObject* enumDict = PyDict_New();
    if (!enumDict) {
      Py_DECREF(dict);
      return nullptr;
    }
    PyObject* members = PyList_New(0);
    if (!members) {
      Py_DECREF(enumDict);
      Py_DECREF(dict);
      return nullptr;
    }
    for (const auto& member : type.members) {
      PyObject* memberDict = PyDict_New();
      if (!memberDict) {
        Py_DECREF(members);
        Py_DECREF(enumDict);
        Py_DECREF(dict);
        return nullptr;
      }
      if (setDictItem(memberDict, "name", PyUnicode_FromString(member.name.c_str())) < 0 ||
          setDictItem(
            memberDict,
            "encoding",
            PyUnicode_FromString(member.encoding.c_str())) < 0 ||
          PyList_Append(members, memberDict) < 0) {
        Py_DECREF(memberDict);
        Py_DECREF(members);
        Py_DECREF(enumDict);
        Py_DECREF(dict);
        return nullptr;
      }
      Py_DECREF(memberDict);
    }
    const auto enumDecl = formatIntentSourceLoc(type.enumDeclLoc);
    if (setDictItem(enumDict, "width", PyLong_FromUnsignedLong(type.enumWidth)) < 0 ||
        setDictItem(enumDict, "decl", PyUnicode_FromString(enumDecl.c_str())) < 0 ||
        setDictItem(enumDict, "members", members) < 0 ||
        setDictItem(dict, "enum", enumDict) < 0) {
      Py_DECREF(dict);
      return nullptr;
    }
  } else if (type.isStruct) {
    PyObject* structDict = PyDict_New();
    if (!structDict) {
      Py_DECREF(dict);
      return nullptr;
    }
    PyObject* fields = PyList_New(0);
    if (!fields) {
      Py_DECREF(structDict);
      Py_DECREF(dict);
      return nullptr;
    }
    for (const auto& field : type.fields) {
      PyObject* fieldDict = PyDict_New();
      if (!fieldDict) {
        Py_DECREF(fields);
        Py_DECREF(structDict);
        Py_DECREF(dict);
        return nullptr;
      }
      if (setDictItem(fieldDict, "name", PyUnicode_FromString(field.name.c_str())) < 0 ||
          setDictItem(
            fieldDict,
            "type",
            PyUnicode_FromString(field.typeName.c_str())) < 0 ||
          setDictItem(fieldDict, "msb", PyLong_FromUnsignedLongLong(field.msb)) < 0 ||
          setDictItem(fieldDict, "lsb", PyLong_FromUnsignedLongLong(field.lsb)) < 0 ||
          PyList_Append(fields, fieldDict) < 0) {
        Py_DECREF(fieldDict);
        Py_DECREF(fields);
        Py_DECREF(structDict);
        Py_DECREF(dict);
        return nullptr;
      }
      Py_DECREF(fieldDict);
    }
    const auto structDecl = formatIntentSourceLoc(type.structDeclLoc);
    if (setDictItem(
          structDict,
          "width",
          PyLong_FromUnsignedLongLong(type.structWidth)) < 0 ||
        setDictItem(
          structDict,
          "decl",
          PyUnicode_FromString(structDecl.c_str())) < 0 ||
        setDictItem(structDict, "fields", fields) < 0 ||
        setDictItem(dict, "struct", structDict) < 0) {
      Py_DECREF(dict);
      return nullptr;
    }
  }
  return dict;
}

naja::NL::NLObject* parseIntentObject(PyObject* arg, const char* functionName) {
  if (IsPySNLDesign(arg)) {
    return PYSNLDesign_O(arg);
  }
  if (IsPySNLDesignObject(arg)) {
    return PYSNLDesignObject_O(arg);
  }
  setError(std::string(functionName) + " expects an SNLDesign or SNLDesignObject");
  return nullptr;
}

}  // namespace

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

static PyObject* liveCompilation(PyObject*, PyObject*) {
  auto* link = naja::NL::SNLSVLiveASTLinkRegistry::getLatest();
  if (!link || !link->getCompilation()) {
    Py_RETURN_NONE;
  }
  return PyCapsule_New(
    link->getCompilation(),
    kFrontendCompilationCapsuleName,
    nullptr);
}

static PyObject* astSymbolOf(PyObject*, PyObject* arg) {
  naja::NL::NLObject* object = nullptr;
  if (IsPySNLDesign(arg)) {
    object = PYSNLDesign_O(arg);
  } else if (IsPySNLDesignObject(arg)) {
    object = PYSNLDesignObject_O(arg);
  } else {
    setError("ast_symbol_of expects an SNLDesign or SNLDesignObject");
    return nullptr;
  }
  auto* link = naja::NL::SNLSVLiveASTLinkRegistry::findForObject(object);
  if (!link) {
    Py_RETURN_NONE;
  }
  auto* symbol = link->getSymbol(object);
  if (!symbol) {
    Py_RETURN_NONE;
  }
  return PyCapsule_New(
    const_cast<slang::ast::Symbol*>(symbol),
    kFrontendSymbolCapsuleName,
    nullptr);
}

static PyObject* snlObjectsOf(PyObject*, PyObject* arg) {
  auto* symbol = static_cast<const slang::ast::Symbol*>(
    PyCapsule_GetPointer(arg, kFrontendSymbolCapsuleName));
  if (!symbol) {
    return nullptr;
  }
  auto* link = naja::NL::SNLSVLiveASTLinkRegistry::findForSymbol(symbol);
  auto* result = PyList_New(0);
  if (!result) {
    return nullptr;
  }
  if (!link) {
    return result;
  }
  for (auto* object : link->getObjects(symbol)) {
    PyObject* pyObject = wrapNLObject(object);
    if (!pyObject) {
      Py_DECREF(result);
      return nullptr;
    }
    if (pyObject == Py_None) {
      Py_DECREF(pyObject);
      continue;
    }
    if (PyList_Append(result, pyObject) < 0) {
      Py_DECREF(pyObject);
      Py_DECREF(result);
      return nullptr;
    }
    Py_DECREF(pyObject);
  }
  return result;
}

static PyObject* intentAvailable(PyObject*, PyObject*) {
  if (naja::NL::SNLSVIntent::available()) {
    Py_RETURN_TRUE;
  }
  Py_RETURN_FALSE;
}

static PyObject* intentTypeOf(PyObject*, PyObject* arg) {
  auto* object = parseIntentObject(arg, "intent_type_of");
  if (!object) {
    return nullptr;
  }
  try {
    return buildIntentTypeDict(naja::NL::SNLSVIntent::typeOf(object));
  } catch (...) {
    Py_RETURN_NONE;
  }
}

static PyObject* intentParametersOf(PyObject*, PyObject* arg) {
  auto* object = parseIntentObject(arg, "intent_parameters_of");
  if (!object) {
    return nullptr;
  }
  try {
    const auto params = naja::NL::SNLSVIntent::parametersOf(object);
    if (!params.valid) {
      Py_RETURN_NONE;
    }
    PyObject* dict = PyDict_New();
    if (!dict) {
      return nullptr;
    }
    PyObject* paramList = PyList_New(0);
    if (!paramList) {
      Py_DECREF(dict);
      return nullptr;
    }
    for (const auto& param : params.params) {
      PyObject* paramDict = buildIntentParamDict(param);
      if (!paramDict) {
        Py_DECREF(paramList);
        Py_DECREF(dict);
        return nullptr;
      }
      if (paramDict == Py_None) {
        Py_DECREF(paramDict);
        continue;
      }
      if (PyList_Append(paramList, paramDict) < 0) {
        Py_DECREF(paramDict);
        Py_DECREF(paramList);
        Py_DECREF(dict);
        return nullptr;
      }
      Py_DECREF(paramDict);
    }
    if (setDictItem(dict, "module", PyUnicode_FromString(params.module.c_str())) < 0 ||
        setDictItem(dict, "parameters", paramList) < 0 ||
        setDictItem(dict, "count", PyLong_FromSize_t(params.params.size())) < 0) {
      Py_DECREF(dict);
      return nullptr;
    }
    return dict;
  } catch (...) {
    Py_RETURN_NONE;
  }
}

static PyObject* intentPackageMember(PyObject*, PyObject* args) {
  const char* package = nullptr;
  const char* member = nullptr;
  if (!PyArg_ParseTuple(args, "ss", &package, &member)) {
    setError("intent_package_member expects a package and member name");
    return nullptr;
  }
  try {
    auto param = naja::NL::SNLSVIntent::packageMember(package, member);
    if (!param.name.empty()) {
      return buildIntentParamDict(param);
    }
    return buildIntentTypeDict(
      naja::NL::SNLSVIntent::packageMemberType(package, member));
  } catch (...) {
    Py_RETURN_NONE;
  }
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
  { "live_compilation", liveCompilation, METH_NOARGS,
    "Return the live frontend compilation capsule for the latest retained SystemVerilog load." },
  { "ast_symbol_of", astSymbolOf, METH_O,
    "Return the live frontend AST symbol capsule associated with an SNL object, or None." },
  { "snl_objects_of", snlObjectsOf, METH_O,
    "Return SNL objects associated with a live frontend AST symbol capsule." },
  { "intent_available", intentAvailable, METH_NOARGS,
    "Return whether curated live SystemVerilog source intent data is available." },
  { "intent_type_of", intentTypeOf, METH_O,
    "Return a plain Python dict describing live source type intent for an SNL object, or None." },
  { "intent_parameters_of", intentParametersOf, METH_O,
    "Return a plain Python dict with live source parameter intent for an SNL object, or None." },
  { "intent_package_member", intentPackageMember, METH_VARARGS,
    "Return a plain Python dict for a live source package member, or None." },
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
  naja::NajaPerf::create(
    naja::NajaPerf::getLogPathFromEnv("NAJA_PERF", "naja_perf.log"),
    "naja_python");

  PyNLUniverse_LinkPyType();
  PyNLID_LinkPyType();
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
  PySNLBundleTerm_LinkPyType();
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
  PySNLLogicalCone_LinkPyType();
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
  PySNLBundleTerms_LinkPyType();
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
  PYTYPE_READY(NLID);
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
  PYTYPE_READY_SUB(SNLBundleTerm, SNLTerm);
  PYTYPE_READY_SUB(SNLBusTerm, SNLTerm);
  PYTYPE_READY_SUB(SNLBitTerm, SNLTerm);
  PYTYPE_READY_SUB(SNLScalarTerm, SNLBitTerm);
  PYTYPE_READY_SUB(SNLBusTermBit, SNLBitTerm);
  PYTYPE_READY_SUB(SNLInstance, SNLDesignObject);
  PYTYPE_READY_SUB(SNLInstTerm, SNLNetComponent);

  PYTYPE_READY(SNLPath);
  PYTYPE_READY(SNLUniquifier);
  PYTYPE_READY(SNLEquipotential);
  PYTYPE_READY(SNLLogicalCone);
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
  PYTYPE_READY(SNLBundleTerms);
  PYTYPE_READY(SNLBundleTermsIterator);
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
  PyModule_AddType(mod, &PyTypeNLID);
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
  PyModule_AddType(mod, &PyTypeSNLBundleTerm);
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
  PyModule_AddType(mod, &PyTypeSNLLogicalCone);

  PyNLID_postModuleInit();
  PySNLTerm_postModuleInit();
  PySNLNet_postModuleInit();
  PySNLLogicalCone_postModuleInit();

  return mod;
}

}
