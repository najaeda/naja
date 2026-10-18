// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLMemoryInterface.h"

#include "PyInterface.h"
#include "PySNLBitTerm.h"

#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLInstance.h"

#include <memory>

namespace PYNAJA {

using namespace naja::NL;

namespace {

struct PyDeleter {
  void operator()(PyObject* object) const { Py_XDECREF(object); }
};
using PyOwned = std::unique_ptr<PyObject, PyDeleter>;

// Consume a new reference, including on failure.
bool setField(PyObject* dictionary, const char* key, PyObject* value) {
  PyOwned owned(value);
  return value && PyDict_SetItemString(dictionary, key, value) == 0;
}

template<typename Values, typename Convert>
PyObject* toList(const Values& values, Convert convert) {
  PyOwned result(PyList_New(values.size()));
  if (!result) return nullptr;
  Py_ssize_t index = 0;
  for (const auto& value : values) {
    auto* item = convert(value);
    if (!item) return nullptr;
    PyList_SET_ITEM(result.get(), index++, item);
  }
  return result.release();
}

PyObject* toTerm(SNLBitTerm* term) {
  if (!term) Py_RETURN_NONE;
  return PySNLBitTerm_Link(term);
}

PyObject* toTerms(const SNLDesignModeling::BitTerms& terms) {
  return toList(terms, toTerm);
}

PyObject* toReadPort(const SNLDesignModeling::MemoryReadPort& port) {
  PyOwned result(PyDict_New());
  if (!result ||
      !setField(result.get(), "address", toTerms(port.address)) ||
      !setField(result.get(), "data", toTerms(port.data)) ||
      !setField(result.get(), "enables", toTerms(port.enables))) return nullptr;
  return result.release();
}

PyObject* toWritePort(const SNLDesignModeling::MemoryWritePort& port) {
  PyOwned result(toReadPort({port.address, port.data, port.enables}));
  if (!result ||
      !setField(result.get(), "mask", toTerms(port.mask)) ||
      !setField(result.get(), "extraWriteInputs", toList(port.extraWriteInputs, toTerms)) ||
      !setField(result.get(), "clock", toTerm(port.clock))) return nullptr;
  return result.release();
}

const char* resetModeName(SNLDesignModeling::MemoryResetMode mode) {
  using Mode = SNLDesignModeling::MemoryResetMode;
  switch (mode) {
    case Mode::None: return "none";
    case Mode::AsyncLow: return "async_low";
    case Mode::AsyncHigh: return "async_high";
    case Mode::SyncLow: return "sync_low";
    case Mode::SyncHigh: return "sync_high";
  }
  throw NLException("Unknown memory reset mode");
}

PyObject* toMemoryInterface(const SNLDesignModeling::MemoryInterface& memory) {
  PyOwned result(PyDict_New());
  if (!result ||
      !setField(result.get(), "width", PyLong_FromSize_t(memory.width)) ||
      !setField(result.get(), "depth", PyLong_FromSize_t(memory.depth)) ||
      !setField(result.get(), "abits", PyLong_FromSize_t(memory.abits)) ||
      !setField(result.get(), "resetMode", PyUnicode_FromString(resetModeName(memory.resetMode))) ||
      !setField(result.get(), "clock", toTerm(memory.clock)) ||
      !setField(result.get(), "reset", toTerm(memory.reset)) ||
      !setField(result.get(), "readPorts", toList(memory.readPorts, toReadPort)) ||
      !setField(result.get(), "writePorts", toList(memory.writePorts, toWritePort))) return nullptr;
  return result.release();
}

}  // namespace

PyObject* PySNLDesign_hasMemoryInterface(PySNLDesign* self) {
  GENERIC_METHOD_HEAD(SNLDesign, "SNLDesign.hasMemoryInterface()")
  TRY
  return PyBool_FromLong(SNLDesignModeling::hasMemoryInterface(selfObject));
  NLCATCH
}

PyObject* PySNLDesign_getMemoryInterface(PySNLDesign* self) {
  GENERIC_METHOD_HEAD(SNLDesign, "SNLDesign.getMemoryInterface()")
  TRY
  return toMemoryInterface(SNLDesignModeling::getMemoryInterface(selfObject));
  NLCATCH
}

PyObject* PySNLInstance_getMemoryInterface(PySNLInstance* self) {
  auto* selfObject = PYSNLInstance_O(self);
  if (!selfObject) {
    setError("SNLInstance.getMemoryInterface(): unbound instance");
    return nullptr;
  }
  TRY
  return toMemoryInterface(SNLDesignModeling::getMemoryInterface(selfObject));
  NLCATCH
}

}  // namespace PYNAJA
