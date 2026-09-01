// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLSequentialModel.h"

#include "PyInterface.h"
#include "PySNLBitTerm.h"

#include "SNLBooleanTree.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"

namespace PYNAJA {

using namespace naja::NL;

namespace {

const char* getStringField(
    PyObject* dictionary, const char* field, bool required) {
  auto* object = PyDict_GetItemString(dictionary, field);
  if (object == nullptr) {
    if (required) {
      setError(std::string("SNLDesign.setSequentialModel() state requires `") +
               field + "`");
    }
    return nullptr;
  }
  if (object == Py_None && not required) {
    return nullptr;
  }
  if (not PyUnicode_Check(object)) {
    setError(std::string("SNLDesign.setSequentialModel() `") + field +
             "` must be a string");
    return nullptr;
  }
  return PyUnicode_AsUTF8(object);
}

SNLDesignModeling::BooleanExpression parseExpression(
    const SNLDesign* design,
    const char* expression,
    const SNLBooleanTree::StateIdentifiers& stateIdentifiers) {
  SNLBooleanTree tree;
  tree.parse(design, expression, stateIdentifiers);
  return tree.getBooleanExpression();
}

bool addStateIdentifier(
    SNLBooleanTree::StateIdentifiers& stateIdentifiers,
    const char* name,
    size_t index,
    bool inverted) {
  if (stateIdentifiers.emplace(name, std::make_pair(index, inverted)).second) {
    return true;
  }
  setError(std::string("SNLDesign.setSequentialModel() duplicate state name `") +
           name + "`");
  return false;
}

bool parseClearPresetValue(
    PyObject* stateObject,
    SNLDesignModeling::SequentialState& state) {
  using Value = SNLDesignModeling::SequentialState::ClearPresetValue;
  const char* value = getStringField(
      stateObject, "clear_preset_value", false);
  if (value == nullptr) {
    return not PyErr_Occurred();
  }
  const std::string valueString(value);
  if (valueString == "zero") state.clearPresetValue = Value::Zero;
  else if (valueString == "one") state.clearPresetValue = Value::One;
  else if (valueString == "hold") state.clearPresetValue = Value::Hold;
  else if (valueString == "toggle") state.clearPresetValue = Value::Toggle;
  else if (valueString == "unknown") state.clearPresetValue = Value::Unknown;
  else {
    setError(
        "SNLDesign.setSequentialModel() `clear_preset_value` must be "
        "zero, one, hold, toggle, or unknown");
    return false;
  }
  return true;
}

}  // namespace

PyObject* PySNLDesign_setSequentialModel(
    PySNLDesign* self, PyObject* args, PyObject* kwargs) {
  const char* clockedOn = nullptr;
  PyObject* statesObject = nullptr;
  PyObject* outputsObject = nullptr;
  const char* kind = "flip_flop";
  static const char* const keywords[] = {
      "clocked_on", "states", "outputs", "kind", nullptr};
  if (not PyArg_ParseTupleAndKeywords(
          args, kwargs, "sOO|s:SNLDesign.setSequentialModel",
          const_cast<char**>(keywords),
          &clockedOn, &statesObject, &outputsObject, &kind)) {
    setError("malformed SNLDesign.setSequentialModel method");
    return nullptr;
  }
  if (not PyList_Check(statesObject) || not PyList_Check(outputsObject)) {
    setError(
        "SNLDesign.setSequentialModel() expects lists for states and outputs");
    return nullptr;
  }
  GENERIC_METHOD_HEAD(SNLDesign, "SNLDesign.setSequentialModel()")

  SNLBooleanTree::StateIdentifiers stateIdentifiers;
  for (Py_ssize_t i = 0; i < PyList_Size(statesObject); ++i) {
    auto* stateObject = PyList_GetItem(statesObject, i);
    if (not PyDict_Check(stateObject)) {
      setError("SNLDesign.setSequentialModel() states must be dictionaries");
      return nullptr;
    }
    const char* name = getStringField(stateObject, "name", true);
    if (name == nullptr ||
        not addStateIdentifier(stateIdentifiers, name, i, false)) {
      return nullptr;
    }
    const char* invertedName = getStringField(
        stateObject, "inverted_name", false);
    if (PyErr_Occurred() ||
        (invertedName != nullptr &&
         not addStateIdentifier(stateIdentifiers, invertedName, i, true))) {
      return nullptr;
    }
  }

  TRY
  SNLDesignModeling::SequentialModel model;
  const std::string kindString(kind);
  if (kindString == "flip_flop") {
    model.kind = SNLDesignModeling::SequentialModel::Kind::FlipFlop;
  } else if (kindString == "latch") {
    model.kind = SNLDesignModeling::SequentialModel::Kind::Latch;
  } else {
    setError(
        "SNLDesign.setSequentialModel() kind must be flip_flop or latch");
    return nullptr;
  }
  model.clockedOn = parseExpression(
      selfObject, clockedOn, stateIdentifiers);

  for (Py_ssize_t i = 0; i < PyList_Size(statesObject); ++i) {
    auto* stateObject = PyList_GetItem(statesObject, i);
    const char* nextState = getStringField(stateObject, "next_state", true);
    if (nextState == nullptr) return nullptr;

    SNLDesignModeling::SequentialState state;
    state.nextState = parseExpression(
        selfObject, nextState, stateIdentifiers);
    const char* clear = getStringField(stateObject, "clear", false);
    if (PyErr_Occurred()) return nullptr;
    if (clear != nullptr) {
      state.clear = parseExpression(selfObject, clear, stateIdentifiers);
    }
    const char* preset = getStringField(stateObject, "preset", false);
    if (PyErr_Occurred()) return nullptr;
    if (preset != nullptr) {
      state.preset = parseExpression(selfObject, preset, stateIdentifiers);
    }
    if (not parseClearPresetValue(stateObject, state)) return nullptr;
    model.states.push_back(std::move(state));
  }

  for (Py_ssize_t i = 0; i < PyList_Size(outputsObject); ++i) {
    auto* outputObject = PyList_GetItem(outputsObject, i);
    if (not PyTuple_Check(outputObject) || PyTuple_Size(outputObject) != 2) {
      setError(
          "SNLDesign.setSequentialModel() outputs must be "
          "(SNLBitTerm, expression) tuples");
      return nullptr;
    }
    auto* termObject = PyTuple_GetItem(outputObject, 0);
    auto* expressionObject = PyTuple_GetItem(outputObject, 1);
    if (not IsPySNLBitTerm(termObject) ||
        not PyUnicode_Check(expressionObject)) {
      setError(
          "SNLDesign.setSequentialModel() outputs must be "
          "(SNLBitTerm, expression) tuples");
      return nullptr;
    }
    auto* term = PYSNLBitTerm_O(termObject);
    if (term->getDesign() != selfObject) {
      setError(
          "SNLDesign.setSequentialModel() output term belongs to "
          "another design");
      return nullptr;
    }
    model.outputs.push_back({
        term,
        parseExpression(
            selfObject, PyUnicode_AsUTF8(expressionObject), stateIdentifiers)});
  }
  SNLDesignModeling::setSequentialModel(selfObject, model);
  NLCATCH
  Py_RETURN_NONE;
}

PyObject* PySNLDesign_hasSequentialModel(PySNLDesign* self) {
  GENERIC_METHOD_HEAD(SNLDesign, "SNLDesign.hasSequentialModel()")
  if (SNLDesignModeling::hasSequentialModel(selfObject)) Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

}  // namespace PYNAJA
