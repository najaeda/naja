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

}  // namespace PYNAJA
