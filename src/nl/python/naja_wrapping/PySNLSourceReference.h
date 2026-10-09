// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Python.h"
#include "SNLRTLInfos.h"

namespace PYNAJA {

inline PyObject* sourceReferenceToPython(const naja::NL::SNLSourceReference& reference) {
  const auto& loc = reference.range;
  return Py_BuildValue("((sIIII)sss)", loc.file.getString().c_str(),
    static_cast<unsigned>(loc.line), static_cast<unsigned>(loc.column),
    static_cast<unsigned>(loc.endLine), static_cast<unsigned>(loc.endColumn),
    reference.provider.getString().c_str(), reference.language.getString().c_str(),
    reference.representation.getString().c_str());
}

inline bool sourceReferenceFromPython(
  PyObject* value, naja::NL::SNLSourceReference& reference) {
  if (!PyTuple_Check(value)) {
    PyErr_SetString(PyExc_TypeError, "Source reference must be a tuple");
    return false;
  }
  const char *file, *provider, *language, *representation;
  PyObject *line, *column, *endLine, *endColumn;
  if (!PyArg_ParseTuple(value, "(sOOOO)sss", &file, &line, &column, &endLine,
      &endColumn, &provider, &language, &representation)) return false;
  const auto parseCoordinate = [](PyObject* object, unsigned long max, auto& result) {
    auto value = PyLong_AsUnsignedLong(object);
    if (PyErr_Occurred()) return false;
    if (value > max) {
      PyErr_SetString(PyExc_OverflowError, "Source coordinate is out of range");
      return false;
    }
    result = value;
    return true;
  };
  auto& loc = reference.range;
  if (!parseCoordinate(line, UINT32_MAX, loc.line) ||
      !parseCoordinate(column, UINT16_MAX, loc.column) ||
      !parseCoordinate(endLine, UINT32_MAX, loc.endLine) ||
      !parseCoordinate(endColumn, UINT16_MAX, loc.endColumn)) return false;
  loc.file = naja::NL::NLName(file);
  reference.provider = naja::NL::NLName(provider);
  reference.language = naja::NL::NLName(language);
  reference.representation = naja::NL::NLName(representation);
  return true;
}

inline PyObject* getSourceDeclaration(const naja::NL::SNLRTLInfos* infos) {
  if (!infos || !infos->getSourceDeclaration()) Py_RETURN_NONE;
  return sourceReferenceToPython(*infos->getSourceDeclaration());
}

inline PyObject* getSourceOrigins(const naja::NL::SNLRTLInfos* infos) {
  auto count = infos ? infos->getSourceOrigins().size() : 0;
  auto* result = PyList_New(count);
  if (!result) return nullptr;
  for (size_t i = 0; i < count; ++i) {
    auto* item = sourceReferenceToPython(infos->getSourceOrigins()[i]);
    if (!item) {
      Py_DECREF(result);
      return nullptr;
    }
    PyList_SET_ITEM(result, i, item);
  }
  return result;
}

template<typename T>
PyObject* setSourceDeclaration(T* object, PyObject* value) {
  auto* infos = object->getRTLInfos();
  if (value == Py_None) {
    if (infos) infos->clearSourceDeclaration();
  } else {
    naja::NL::SNLSourceReference reference;
    if (!sourceReferenceFromPython(value, reference)) return nullptr;
    if (!infos) infos = naja::NL::SNLRTLInfos::create(object);
    infos->setSourceDeclaration(reference);
  }
  Py_RETURN_NONE;
}

template<typename T>
PyObject* addSourceOrigin(T* object, PyObject* value) {
  naja::NL::SNLSourceReference reference;
  if (!sourceReferenceFromPython(value, reference)) return nullptr;
  auto* infos = object->getRTLInfos();
  if (!infos) infos = naja::NL::SNLRTLInfos::create(object);
  infos->addSourceOrigin(reference);
  Py_RETURN_NONE;
}

}  // namespace PYNAJA
