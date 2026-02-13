// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLDB.h"

#include <Python.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "NLUniverse.h"
#include "NLDB.h"
#include "SNLCapnP.h"
#include "SNLLibertyConstructor.h"
#include "SNLSVConstructor.h"
#include "SNLUtils.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"

#include "PyInterface.h"
#include "PyNLUniverse.h"
#include "PyNLLibraries.h"
#include "PyNLLibrary.h"
#include "PySNLDesign.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(NLDB, function)

static PyObject* PyNLDB_create(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.create", &arg)) {
    setError("malformed NLDB create");
    return nullptr;
  }
  if (not IsPyNLUniverse(arg)) {
    setError("NLDB create argument should be a NLUniverse");
    return nullptr;
  }
  auto universe = PYNLUNIVERSE_O(arg);
  NLDB* db = nullptr;
  if (universe == nullptr) {
    setError("NLDB create NLUniverse is null");
    return nullptr;
  }
  TRY
  db = NLDB::create(universe);
  NLCATCH // LCOV_EXCL_LINE should throw if universe is null, already checked
  return PyNLDB_Link(db);
}

static PyObject* PyNLDB_loadNajaIF(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.loadNajaIF", &arg)) {
    setError("malformed NLDB loadNajaIF");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    std::ostringstream oss;
    oss << "NLDB loadNajaIF argument should be a file path, got: "
      << getStringForPyObject(arg);
    setError(oss.str());
    return nullptr;
  }
  NLDB* db = nullptr;
  const std::filesystem::path path(PyUnicode_AsUTF8(arg));
  if (NLUniverse::get() == nullptr) {
    NLUniverse::create();
  }
  TRY 
  db = SNLCapnP::load(path);
  NLUniverse::get()->setTopDesign(db->getTopDesign());
  NLCATCH
  return PyNLDB_Link(db);
}

PyObject* PyNLDB_dumpNajaIF(PyNLDB* self, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.dumpNajaIF", &arg)) {
    setError("malformed NLDB dumpNajaIF");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    std::ostringstream oss;
    oss << "NLDB dumpNajaIF argument should be a file path, got:"
      << getStringForPyObject(arg);
    setError(oss.str());
    return nullptr;
  }
  METHOD_HEAD("NLDB.dumpNajaIF()")
  SNLCapnP::dump(self->object_, PyUnicode_AsUTF8(arg));
  Py_RETURN_NONE;
}

PyObject* PyNLDB_loadLibertyPrimitives(PyNLDB* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.loadLibertyPrimitives", &arg0)) {
    setError("malformed NLDB loadLibertyPrimitives");
    return nullptr;
  }
  if (not PyList_Check(arg0)) {
    setError("malformed SNLDesign.loadLibertyPrimitives method");
    return nullptr;
  }
  METHOD_HEAD("NLDB.loadLibertyPrimitives()")
  NLLibrary* primitivesLibrary = nullptr;
  primitivesLibrary = selfObject->getLibrary(NLName("PRIMS"));
  if (primitivesLibrary == nullptr) {
    primitivesLibrary =
      NLLibrary::create(selfObject, NLLibrary::Type::Primitives, NLName("PRIMS"));
  }
  SNLLibertyConstructor::Paths paths;
  paths.reserve(static_cast<size_t>(PyList_Size(arg0)));
  for (int i = 0; i < PyList_Size(arg0); ++i) {
    PyObject* object = PyList_GetItem(arg0, i);
    if (not PyUnicode_Check(object)) {
      setError("NLDB loadLibertyPrimitives argument should be a file path");
      return nullptr;
    }
    std::string pathStr = PyUnicode_AsUTF8(object);
    paths.emplace_back(pathStr);
  }
  // LCOV_EXCL_START
  TRY
  SNLLibertyConstructor constructor(primitivesLibrary);
  constructor.construct(paths);
  NLCATCH
  // LCOV_EXCL_STOP
  Py_RETURN_NONE;
}

PyObject* PyNLDB_loadVerilog(PyNLDB* self, PyObject* args, PyObject* kwargs) {
  PyObject* files = nullptr;
  int keep_assigns = 1;  // Default: true
  int allow_unknown_designs = 0; // Default: false
  int preprocess_enabled = 0; // Default: false
  PyObject* conflicting_design_name_policy = nullptr; // Optional: string

  static const char* const kwords[] = {
    "files", "keep_assigns", "allow_unknown_designs", "preprocess_enabled",
    "conflicting_design_name_policy",
    nullptr
  };

  if (not PyArg_ParseTupleAndKeywords(
    args, kwargs, "O|pppO:NLDB.loadVerilog",
    const_cast<char**>(kwords),
    &files, &keep_assigns, &allow_unknown_designs, &preprocess_enabled,
    &conflicting_design_name_policy)) {
    setError("malformed NLDB loadVerilog");
    return nullptr;
  }

  if (not PyList_Check(files)) {
    setError("malformed NLDB.loadVerilog method");
    return nullptr;
  }
  NLDB* db = self->object_;
  SNLDesign* top = nullptr;
  TRY
  //loadVerilog can be called multiple times
  //last one gets top
  NLLibrary* designLibrary = db->getLibrary(NLName("DESIGN"));
  if (designLibrary == nullptr) {
    designLibrary = NLLibrary::create(db, NLName("DESIGN"));
  }
  SNLVRLConstructor constructor(designLibrary);
  if (allow_unknown_designs) {
    constructor.config_.allowUnknownDesigns_ = true;
  }
  if (preprocess_enabled) {
    constructor.config_.preprocessEnabled_ = true;
  }
  // Conflicting design name policy (optional)
  if (conflicting_design_name_policy != nullptr && conflicting_design_name_policy != Py_None) {
    using Policy = SNLVRLConstructor::Config::ConflictingDesignNamePolicy;

    if (!PyUnicode_Check(conflicting_design_name_policy)) {
      std::ostringstream oss;
      oss << "NLDB.loadVerilog: conflicting_design_name_policy must be a str, got: "
          << getStringForPyObject(conflicting_design_name_policy);
      setError(oss.str());
      return nullptr;
    }

    const std::string s = PyUnicode_AsUTF8(conflicting_design_name_policy);
    if (s == "forbid") {
      constructor.config_.conflictingDesignNamePolicy_ = Policy::Forbid;
    } else if (s == "first") {
      constructor.config_.conflictingDesignNamePolicy_ = Policy::FirstOne;
    } else if (s == "last") {
      constructor.config_.conflictingDesignNamePolicy_ = Policy::LastOne;
    } else if (s == "verify") {
      constructor.config_.conflictingDesignNamePolicy_ = Policy::VerifyEquality;
    } else {
      std::ostringstream oss;
      oss << "NLDB.loadVerilog: invalid conflicting_design_name_policy '" << s
          << "' (expected one of: forbid, first, last, verify)";
      setError(oss.str());
      return nullptr;
    }
  }
  using Paths = std::vector<std::filesystem::path>;
  Paths inputPaths;
  for (int i = 0; i < PyList_Size(files); ++i) {
    PyObject* object = PyList_GetItem(files, i);
    if (not PyUnicode_Check(object)) {
      setError("NLDB loadVerilog argument should be a file path");
      return nullptr;
    }
    std::string pathStr = PyUnicode_AsUTF8(object);
    const std::filesystem::path path(pathStr);
    inputPaths.push_back(path);
  }
  try {
    constructor.construct(inputPaths);
  } catch (const std::exception& e) {
    const auto location = constructor.getCurrentLocation();
    std::ostringstream error;
    error << "Error while parsing Verilog: "
      << location.currentPath_.string() << ":"
      << location.line_ << ":"
      << location.column_ << ": ";
    setError(error.str() + e.what());
    return nullptr;
  }
  if (not keep_assigns) {
    db->mergeAssigns();
  }
  top = SNLUtils::findTop(designLibrary);
  if (top) {
    NLUniverse::get()->setTopDesign(top);
    NLUniverse::get()->setTopDB(top->getDB());
  } else {
    setError("No top design was found after parsing verilog"); //LCOV_EXCL_LINE
    return nullptr; //LCOV_EXCL_LINE
  }
  NLCATCH
  return PySNLDesign_Link(top);
}

PyObject* PyNLDB_loadSystemVerilog(PyNLDB* self, PyObject* args, PyObject* kwargs) {
  PyObject* files = nullptr;
  int keep_assigns = 1;  // Default: true
  PyObject* elaborated_ast_json_path = nullptr;  // Optional: string
  int pretty_print_elaborated_ast_json = 1;  // Default: true
  int include_source_info_in_elaborated_ast_json = 1;  // Default: true

  static const char* const kwords[] = {
    "files", "keep_assigns", "elaborated_ast_json_path",
    "pretty_print_elaborated_ast_json", "include_source_info_in_elaborated_ast_json",
    nullptr
  };

  if (not PyArg_ParseTupleAndKeywords(
    args, kwargs, "O|pOpp:NLDB.loadSystemVerilog",
    const_cast<char**>(kwords),
    &files, &keep_assigns, &elaborated_ast_json_path,
    &pretty_print_elaborated_ast_json,
    &include_source_info_in_elaborated_ast_json)) {
    setError("malformed NLDB loadSystemVerilog");
    return nullptr;
  }

  if (not PyList_Check(files)) {
    setError("malformed NLDB.loadSystemVerilog method");
    return nullptr;
  }

  NLDB* db = self->object_;
  SNLDesign* top = nullptr;
  TRY
  // SystemVerilog parsing currently enforces unique design names in the library.
  NLLibrary* designLibrary = db->getLibrary(NLName("DESIGN"));
  if (designLibrary == nullptr) {
    designLibrary = NLLibrary::create(db, NLName("DESIGN"));
  }
  SNLSVConstructor constructor(designLibrary);
  SNLSVConstructor::ConstructOptions options;
  options.prettyPrintElaboratedASTJson = pretty_print_elaborated_ast_json;
  options.includeSourceInfoInElaboratedASTJson =
    include_source_info_in_elaborated_ast_json;

  if (elaborated_ast_json_path != nullptr &&
      elaborated_ast_json_path != Py_None) {
    if (not PyUnicode_Check(elaborated_ast_json_path)) {
      std::ostringstream oss;
      oss << "NLDB.loadSystemVerilog: elaborated_ast_json_path must be a str, got: "
          << getStringForPyObject(elaborated_ast_json_path);
      setError(oss.str());
      return nullptr;
    }
    options.elaboratedASTJsonPath =
      std::filesystem::path(PyUnicode_AsUTF8(elaborated_ast_json_path));
  }

  using Paths = std::vector<std::filesystem::path>;
  Paths inputPaths;
  for (int i = 0; i < PyList_Size(files); ++i) {
    PyObject* object = PyList_GetItem(files, i);
    if (not PyUnicode_Check(object)) {
      setError("NLDB loadSystemVerilog argument should be a file path");
      return nullptr;
    }
    std::string pathStr = PyUnicode_AsUTF8(object);
    const std::filesystem::path path(pathStr);
    inputPaths.push_back(path);
  }

  try {
    constructor.construct(inputPaths, options);
  } catch (const std::exception& e) {
    std::ostringstream error;
    error << "Error while parsing SystemVerilog: ";
    setError(error.str() + e.what());
    return nullptr;
  }

  if (not keep_assigns) {
    db->mergeAssigns();
  }
  top = SNLUtils::findTop(designLibrary);
  if (top) {
    NLUniverse::get()->setTopDesign(top);
    NLUniverse::get()->setTopDB(top->getDB());
  } else {
    setError("No top design was found after parsing systemverilog");
    return nullptr;
  }
  NLCATCH
  return PySNLDesign_Link(top);
}

PyObject* PyNLDB_dumpVerilog(PyNLDB* self, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:NLDB.dumpVerilog", &arg)) {
    setError("malformed NLDB dumpVerilog");
    return nullptr;
  }
  if (not PyUnicode_Check(arg)) {
    setError("NLDB dumpVerilog argument should be a file path");
    return nullptr;
  }
  std::ofstream output(PyUnicode_AsUTF8(arg));
  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.dumpDesign(self->object_->getTopDesign(), output);
  Py_RETURN_NONE;
}

PyObject* PyNLDB_getLibrary(PyNLDB* self, PyObject* arg) {
  NLLibrary* lib = nullptr;
  METHOD_HEAD("NLDB.getLibrary()")
  if (PyUnicode_Check(arg)) {
    const char* name = PyUnicode_AsUTF8(arg);
    lib = selfObject->getLibrary(NLName(name));
  } else if (PyLong_Check(arg)) {
    int index = PyLong_AsLong(arg);
    lib = selfObject->getLibrary(index);
  } else {
      setError("invalid number of parameters for getLibrary.");
      return nullptr;
  }
  return PyNLLibrary_Link(lib);
}

DirectGetNumericMethod(PyNLDB_getID, getID, PyNLDB, NLDB)

GetBoolAttribute(NLDB, isTopDB)
GetObjectMethod(NLDB, SNLDesign, getTopDesign)

GetContainerMethod(NLDB, NLLibrary*, NLLibraries, Libraries)
GetContainerMethod(NLDB, NLLibrary*, NLLibraries, GlobalLibraries)
GetContainerMethod(NLDB, NLLibrary*, NLLibraries, PrimitiveLibraries)

DBoDestroyAttribute(PyNLDB_destroy, PyNLDB)

PyMethodDef PyNLDB_Methods[] = {
  { "create", (PyCFunction)PyNLDB_create, METH_VARARGS | METH_STATIC,
    "create a NLDB."},
  { "getID", (PyCFunction)PyNLDB_getID, METH_NOARGS,
    "get the NLDB ID."},
  { "isTopDB", (PyCFunction)PyNLDB_isTopDB, METH_NOARGS,
    "Returns True if the NLDB is the top DB."},
  { "getTopDesign", (PyCFunction)PyNLDB_getTopDesign, METH_NOARGS,
    "get the top design."},
  { "loadNajaIF", (PyCFunction)PyNLDB_loadNajaIF, METH_VARARGS | METH_STATIC,
    "create a NLDB from NajaIF format."},
  { "dumpNajaIF", (PyCFunction)PyNLDB_dumpNajaIF, METH_VARARGS,
    "dump this NLDB to NajaIF format."},
  { "loadLibertyPrimitives", (PyCFunction)PyNLDB_loadLibertyPrimitives, METH_VARARGS,
    "import primitives from Liberty format."},
  { "loadVerilog", (PyCFunction)PyNLDB_loadVerilog, METH_VARARGS|METH_KEYWORDS,
    "create a design from Verilog format.\n\n"
    "Args:\n"
    "  files (list[str]): input Verilog files\n"
    "  keep_assigns (bool, optional): keep continuous assigns (default True)\n"
    "  allow_unknown_designs (bool, optional): create unknown modules as blackboxes (default False)\n"
    "  preprocess_enabled (bool, optional): enable Verilog preprocessing (default False)\n"
    "  conflicting_design_name_policy (str, optional): how to handle duplicate module names in the same library. "
    "Accepted values: 'forbid' (default), 'first', 'last', 'verify'."},
  { "loadSystemVerilog", (PyCFunction)PyNLDB_loadSystemVerilog, METH_VARARGS|METH_KEYWORDS,
    "create a design from SystemVerilog format.\n\n"
    "Args:\n"
    "  files (list[str]): input SystemVerilog files\n"
    "  keep_assigns (bool, optional): keep continuous assigns (default True)\n"
    "  elaborated_ast_json_path (str, optional): dump Slang elaborated AST JSON to this path\n"
    "  pretty_print_elaborated_ast_json (bool, optional): pretty-print AST JSON (default True)\n"
    "  include_source_info_in_elaborated_ast_json (bool, optional): include source info in AST JSON (default True)."},
  { "dumpVerilog", (PyCFunction)PyNLDB_dumpVerilog, METH_VARARGS,
    "dump this NLDB to SNL format."},
  { "getLibrary", (PyCFunction)PyNLDB_getLibrary, METH_O,
    "retrieve a NLLibrary."},
  { "getLibraries", (PyCFunction)PyNLDB_getLibraries, METH_NOARGS,
    "iterate on this NLDB SNLLibraries."},
  { "getGlobalLibraries", (PyCFunction)PyNLDB_getGlobalLibraries, METH_NOARGS,
    "iterate on all the Libraries owned (directly or indirectly) by this NLDB."},
  { "getPrimitiveLibraries", (PyCFunction)PyNLDB_getPrimitiveLibraries, METH_NOARGS,
    "iterate on all the primitive Libraries owned (directly or indirectly) by this NLDB."},
  { "destroy", (PyCFunction)PyNLDB_destroy, METH_NOARGS,
    "destroy this NLDB."},
  {NULL, NULL, 0, NULL} /* sentinel */
};

DBoDeallocMethod(NLDB)

DBoLinkCreateMethod(NLDB)
PyTypeNLFinalObjectWithNLIDLinkPyType(NLDB)
PyTypeObjectDefinitions(NLDB)

}  // namespace PYNAJA
