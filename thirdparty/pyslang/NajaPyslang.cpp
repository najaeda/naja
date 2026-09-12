// SPDX-FileCopyrightText: Michael Popoloski
// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: MIT

#include "pyslang.h"

#include <filesystem>
#include <string>

#include "NajaPyslangAPI.h"
#include "NajaSlangRuntime.h"
#include "slang/ast/Compilation.h"
#include "slang/ast/Symbol.h"
#include "slang/util/VersionInfo.h"

void registerAnalysis(py::module_& m, py::module_& ast);
void registerAST(py::module_& m);
void registerCompilation(py::module_& m, py::module_& ast, py::module_& driver);
void registerExpressions(py::module_& m);
void registerNumeric(py::module_& m);
void registerDiagnostics(py::module_& m);
void registerText(py::module_& m);
void registerUtil(py::module_& m);
void registerStatements(py::module_& m);
void registerSymbols(py::module_& m);
void registerSyntax(py::module_& syntax, py::module_& parsing);
void registerSyntaxNodes0(py::module_& m);
void registerSyntaxNodes1(py::module_& m);
void registerSyntaxNodes2(py::module_& m);
void registerSyntaxNodes3(py::module_& m);
void registerSyntaxFactory(py::module_& m);
void registerTypes(py::module_& m);

namespace {

PyObject* wrapCompilation(void* opaqueCompilation, PyObject* owner) {
  try {
    if (opaqueCompilation == nullptr) {
      PyErr_SetString(PyExc_ReferenceError, "cannot wrap a null slang Compilation");
      return nullptr;
    }
    if (owner == nullptr) {
      PyErr_SetString(PyExc_TypeError, "slang Compilation views require an owner");
      return nullptr;
    }
    auto* compilation = static_cast<slang::ast::Compilation*>(opaqueCompilation);
    return py::cast(compilation, py::return_value_policy::reference_internal,
                    py::handle(owner)).release().ptr();
  }
  catch (py::error_already_set& error) {
    error.restore();
  }
  catch (const std::exception& error) {
    PyErr_SetString(PyExc_RuntimeError, error.what());
  }
  catch (...) {
    PyErr_SetString(PyExc_RuntimeError, "unknown error while wrapping slang Compilation");
  }
  return nullptr;
}

PyObject* wrapSymbol(const void* opaqueSymbol, PyObject* owner) {
  try {
    if (opaqueSymbol == nullptr) {
      PyErr_SetString(PyExc_ReferenceError, "cannot wrap a null slang Symbol");
      return nullptr;
    }
    if (owner == nullptr) {
      PyErr_SetString(PyExc_TypeError, "slang Symbol views require an owner");
      return nullptr;
    }
    auto* symbol = static_cast<const slang::ast::Symbol*>(opaqueSymbol);
    return py::cast(symbol, py::return_value_policy::reference_internal,
                    py::handle(owner)).release().ptr();
  }
  catch (py::error_already_set& error) {
    error.restore();
  }
  catch (const std::exception& error) {
    PyErr_SetString(PyExc_RuntimeError, error.what());
  }
  catch (...) {
    PyErr_SetString(PyExc_RuntimeError, "unknown error while wrapping slang Symbol");
  }
  return nullptr;
}

int unwrapSymbol(PyObject* object, const void** outSymbol) {
  if (outSymbol == nullptr) {
    PyErr_SetString(PyExc_SystemError, "out_symbol must not be null");
    return -1;
  }
  *outSymbol = nullptr;
  if (object == nullptr) {
    PyErr_SetString(PyExc_TypeError, "expected a najaeda.pyslang Symbol");
    return -1;
  }
  try {
    const auto* symbol = py::cast<const slang::ast::Symbol*>(py::handle(object));
    if (symbol == nullptr) {
      PyErr_SetString(PyExc_ReferenceError, "slang Symbol is no longer alive");
      return -1;
    }
    *outSymbol = symbol;
    return 0;
  }
  catch (const py::cast_error&) {
    PyErr_SetString(PyExc_TypeError, "expected a najaeda.pyslang Symbol");
  }
  catch (py::error_already_set& error) {
    error.restore();
  }
  catch (const std::exception& error) {
    PyErr_SetString(PyExc_RuntimeError, error.what());
  }
  catch (...) {
    PyErr_SetString(PyExc_RuntimeError, "unknown error while unwrapping slang Symbol");
  }
  return -1;
}

const NajaPyslangAPI_v1 kPyslangAPI = {
  NAJA_PYSLANG_API_VERSION,
  sizeof(NajaPyslangAPI_v1),
  NajaSlang_GetNativeBuildID(),
  NajaSlang_GetRuntimeIdentity(),
  wrapCompilation,
  wrapSymbol,
  unwrapSymbol,
};

py::dict slangBuildInfo() {
  py::dict info;
  info["provider"] = "najaeda.pyslang";
  info["api_version"] = NAJA_PYSLANG_API_VERSION;
  info["slang_version"] = slang::VersionInfo::getVersionString();
  info["git_hash"] = std::string(slang::VersionInfo::getHash());
  info["build_id"] = NajaSlang_GetNativeBuildID();
  info["runtime_kind"] = "shared";
  return info;
}

void registerNajaInterop(py::module_& m) {
  m.def("slang_build_info", &slangBuildInfo,
        "Return diagnostic information for Naja's bundled slang runtime.");

  PyObject* capsule = PyCapsule_New(
    const_cast<NajaPyslangAPI_v1*>(&kPyslangAPI),
    NAJA_PYSLANG_API_CAPSULE_NAME,
    nullptr);
  if (capsule == nullptr) {
    throw py::error_already_set();
  }
  if (PyModule_AddObject(m.ptr(), "_C_API", capsule) < 0) {
    Py_DECREF(capsule);
    throw py::error_already_set();
  }
}

}  // namespace

PYBIND11_MODULE(pyslang, m) {
  m.doc() = "Naja-owned Python bindings for the pinned slang runtime";
  m.attr("__version__") = slang::VersionInfo::getVersionString();

  auto ast = m.def_submodule("ast", "AST types: symbols, expressions, statements, types");
  auto syntax = m.def_submodule("syntax", "Syntax tree nodes and utilities");
  auto parsing = m.def_submodule("parsing", "Lexer, parser, preprocessor types");
  auto analysis = m.def_submodule("analysis", "Code analysis utilities");
  auto driver = m.def_submodule("driver", "Compilation driver");

  registerAnalysis(analysis, ast);
  registerAST(ast);
  registerCompilation(m, ast, driver);
  registerExpressions(ast);
  registerNumeric(m);
  registerDiagnostics(m);
  registerText(m);
  registerUtil(m);
  registerStatements(ast);
  registerSymbols(ast);
  registerSyntax(syntax, parsing);
  registerSyntaxNodes0(syntax);
  registerSyntaxNodes1(syntax);
  registerSyntaxNodes2(syntax);
  registerSyntaxNodes3(syntax);
  registerSyntaxFactory(syntax);
  registerTypes(ast);

  py::register_exception_translator([](std::exception_ptr error) {
    try {
      if (error) {
        std::rethrow_exception(error);
      }
    }
    catch (const std::filesystem::filesystem_error& exception) {
      const auto code = exception.code();
      const auto path1 = exception.path1().string();
      const auto path2 = exception.path2().string();
      PyErr_SetObject(PyExc_IOError,
                      py::make_tuple(code.value(), code.message(),
                                     path1.empty() ? py::none() : py::cast(path1), code.value(),
                                     path2.empty() ? py::none() : py::cast(path2)).ptr());
    }
  });

  registerNajaInterop(m);
}
